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
	fprintf(stderr, "nb__desert_init, module = %lu\n", (uint64_t)m);
	return;
}

void nb__desert_deinit(void) {
	return;
}

char nb__reuse_mtu_buffer[DESERT_MTU];

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
	assert(readbuflen < READ_BUF_LEN); // If this ain't true we have problems.
	int totalSize = 0; // Combined size of the data portion of all the packets.
	int lastPacket = 0;
	*size = 0;
	// fprintf(stderr, "nb__poll_packet, readbuflen = %lu\n", readbuflen);
	for (int i = 0; i < readbuflen; i++) {
		Packet* p = readbuf[i];
		int packetLen = p->datalen();
		if (totalSize + packetLen > DESERT_MTU) {
			lastPacket = i;
			break;
		}
		totalSize += packetLen;
	}
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
		// 	// This packet has been freed, so we can skip it.
			continue;
		}
		size_t len = p->datalen();
		memcpy(scratch + used, (char*) p->accessdata(), len);
		Packet::free(p);
		used += len;
	}
	// std::cout << "finished copying packets\n";
	assert(totalSize < DESERT_MTU);
	char* ret = (char*)malloc(DESERT_MTU + headroom);
	memcpy(ret + headroom, scratch, DESERT_MTU);
	*size = totalSize;
	// Reset the readbuf
	for (size_t i = 0; (i < readbuflen - lastPacket); i++) {
		// Copy the packet at the top of the buffer to the first idx ()
		readbuf[i] = readbuf[i+lastPacket];
	}
	m->setRecvBufLen(readbuflen - lastPacket);
	return ret;
}

static int uidcnt_ = 0;
/**
 * @brief Sends a packet down a layer
 * 
 * @param buff Packet to send
 * @param len 
 * @return int 0 or 1 always
 */
int nb__send_packet(char* buff, int len) {
	Packet *p = Packet::alloc();
	hdr_cmn *ch = hdr_cmn::access(p);
	ch->uid() = uidcnt_++;
	ch->ptype() = 2; //CBR style header Fwiw.
	ch->size() = len;
	p->allocdata(len);
	unsigned char* pktdata_p = p->accessdata();
	memcpy((char*) pktdata_p, buff, len);
	// fprintf(stderr, "nb__send_packet, len = %i\n", len);
	assert(!memcmp((char*) pktdata_p, buff, len));
	assert(len == p->datalen());
	m->senddown(p,0);
	return 0;
}


char* nb__request_send_buffer(void) {
	return (char*) malloc(DESERT_MTU);
}
void nb__return_send_buffer(char* p) {
	free(p);
}
