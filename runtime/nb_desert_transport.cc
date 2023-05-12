#include "nb_runtime.h"
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>
#include <vector>

#include "packet.h"
#include "module.h"
#include "scheduler.h"

#include "nb_p.h"

static Nb_pModule *m;

#define DESERT_MTU (1024)
int nb__desert_simulate_out_of_order = 0;
int nb__desert_simulate_packet_drop = 0;
static char out_of_order_store[DESERT_MTU];
static int out_of_order_len = 0;
#define OUT_OF_ORDER_CHANCE (5)
#define PACKET_DROP_CHANCE (5)

void nb__desert_init(void *_m) {
	// connect mode
	m = (Nb_pModule*)_m;
	fprintf(stdout, "nb__desert_init, module = %lu\n", (uint64_t)m);
	m->setRecvBufLen(0);
	return;
}

void nb__desert_deinit(void) {
	return;
}

char nb__reuse_mtu_buffer[DESERT_MTU];

static int rx_pkt_count = 0;
static size_t rx_pkt_bytes = 0;
/**
 * @brief polls packets from those read into nb_read_pkt_buf from the recv() function in nb_p.c
 * 
 * @param size 
 * @param headroom 
 * @return char* 
 */
char* nb__poll_packet(int* size, int headroom) {
	size_t readbuflen = 10000;
	// This holds packets from the application level recv() function, which just enques them into the read buffer.
	Packet** readbuf = m->getRecvBuf(&readbuflen);
	if(readbuflen > READ_BUF_LEN) {assert(false);} // If this ain't true we have problems.
	int totalSize = 0; // Combined size of the data portion of all the packets.
	int lastPacket = 0;
	*size = 0;
	for (int i = 0; i < readbuflen; i++) {
		Packet* p = readbuf[i];
		int packetLen = p->datalen();
		if (totalSize + packetLen > DESERT_MTU) {
			lastPacket = i;
			break;
		}
		fprintf(stdout, "packet %i has len %i\n", i, packetLen);
		totalSize += packetLen;
	}
	lastPacket = (lastPacket == 0) ? readbuflen : lastPacket;
	
	char* scratch[DESERT_MTU];
	if (totalSize == 0) {
		*size = 0;
		m->setRecvBufLen(0);
		return NULL;
	}
	size_t used = 0;
	// Take from the top and copy down.
	for (size_t i = 0; (i < lastPacket); i++) {
		Packet* p = readbuf[i];
		if (p->datalen() == 0) {
			// This packet has no data, so we can skip it.
			continue;
		}
		size_t len = p->datalen();
		memcpy(scratch + used, (char*) p->accessdata(), len);
		Packet::free(p);
		used += len;
	}
	// if (readbuf[lastPacket]->datalen() > DESERT_MTU) { // special case, take all that's left from the last packet.
	// 	// This packet is too big to fit in a single packet. This should really be banned on the tx side too.
	// 	std::cerr << "Packet too big to fit in a single packet.\n";
	// 	memcpy(scratch+used, (char*) readbuf[lastPacket]->accessdata(), DESERT_MTU - used);
	// 	used = DESERT_MTU;
	// 	scratch[lastPacket] = (char*) readbuf[lastPacket]->accessdata();
	// 	readbuf[lastPacket];
	// 	Packet *p = Packet::alloc();
	// 	hdr_cmn *ch = hdr_cmn::access(p);
	// 	ch->uid() = readbuf[lastPacket]->uid_;
	// 	ch->ptype() = 2; //CBR style header Fwiw.
	// 	ch->size() = readbuf[lastPacket]->datalen() - (DESERT_MTU - used);
	// 	p->allocdata(readbuf[lastPacket]->datalen() - (DESERT_MTU - used));
	// 	unsigned char* pktdata_p = p->accessdata();
	// 	memcpy((char*) pktdata_p, readbuf[lastPacket]->accessdata(), readbuf[lastPacket]->datalen() - (DESERT_MTU - used));
	// 	// Packet::free(readbuf[lastPacket]);
	// 	readbuf[lastPacket] = p;
	// }
	// std::cout << "finished copying packets\n";
	assert(totalSize <= DESERT_MTU);
	char* ret = (char*)malloc(DESERT_MTU + headroom);
	memcpy(ret + headroom, scratch, DESERT_MTU);
	*size = totalSize;
	scratch[used] = 0; // Null terminate the string for debugging purposes.
	// Reset the readbuf
	for (size_t i = 0; (i < readbuflen - lastPacket); i++) {
		// Copy the packet at the top of the buffer to the first idx ()
		readbuf[i] = readbuf[i+lastPacket];
	}
	m->setRecvBufLen(readbuflen - lastPacket);
	rx_pkt_count+=lastPacket;
	rx_pkt_bytes += used;
	fprintf(stdout, "rxbytes = %lu, rxpackets = %i\n", rx_pkt_bytes, rx_pkt_count);
	return ret;
}

static int uidcnt_ = 0;
static int send_cnt = 0;
static size_t sent_bytes = 0;
/**
 * @brief Sends a packet down a layer
 * 
 * @param buff Packet to send
 * @param len 
 * @return int 0 or 1 always
 */
int nb__send_packet(char* buff, int len) {
	if (len > DESERT_MTU) { // Recurisvely call till done.
		for (int i = 0; i * DESERT_MTU < len; i++) {
			int thislen = DESERT_MTU;
			if (i * DESERT_MTU + thislen > len) {
				thislen = len - i * DESERT_MTU;
			}
			nb__send_packet(buff + i * DESERT_MTU, thislen);
		}
		return 0;
	} else {
		Packet *p = Packet::alloc();
		hdr_cmn *ch = hdr_cmn::access(p);
		ch->uid() = uidcnt_++;
		ch->ptype() = 2; //CBR style header Fwiw.
		ch->size() = len;
		p->allocdata(len);
		unsigned char* pktdata_p = p->accessdata();
		memcpy((char*) pktdata_p, buff, len);
		assert(!memcmp((char*) pktdata_p, buff, len));
		assert(len == p->datalen());
		m->senddown(p,0);
		send_cnt++;
		sent_bytes += len;
		fprintf(stdout, "NB LLSend = %i, LLBytes = %lu, bytesTx'd = %lu\n", send_cnt, sent_bytes, sent_bytes - len);
		return 0;
	}
}


char* nb__request_send_buffer(void) {
	return (char*) malloc(DESERT_MTU);
}
void nb__return_send_buffer(char* p) {
	free(p);
}

double nb__desert_get_time(void) {
	double t = Scheduler::instance().clock();
	return t;
}
