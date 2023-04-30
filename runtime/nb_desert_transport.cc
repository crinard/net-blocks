#include "nb_runtime.h"
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <iostream>

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
	int len;
	static char temp_buf[DESERT_MTU];
	// std::vector<Packet*> readbuf = m->getNBReadBuffer();
	//TODO: Replace w/ real code

	Packet p_fake = Packet();

	std::vector<Packet*> readbuf;
	readbuf.push_back(&p_fake);

	*size = 0;//TODO: Should assert?
	char ** ret = (char**) malloc(sizeof(char*) * readbuf.size());
	for (int i = 0; i < readbuf.size(); i++) {
		Packet* p = readbuf[i];
		// unsigned char* data = p->userdata();
		//TODO: how to handle multiple packets from different sources?
		char* data[DESERT_MTU];
		if  (1) {//(p->size() > 0) {
			char* buf = (char*)malloc(DESERT_MTU + headroom);
			memcpy(buf + headroom, data , DESERT_MTU);
			ret[i] = buf;
			*size++;
		}
	}
	readbuf.clear();
	return ret[0]; //TODO: fix this
}

/**
 * @brief Sends a packet down a layer
 * 
 * @param buff Packet to send
 * @param len 
 * @return int 0 or 1 always
 */
int nb__send_packet(char* buff, int len) {
	std::cout << "nb__send_packet" << std::endl;
	Packet p = Packet();
	p.allocdata(len);
	unsigned char* pktdata_p = p.accessdata();
	memcpy(pktdata_p, buff, len);
	m->senddown(&p,0);
	return 0;
}


char* nb__request_send_buffer(void) {
	return (char*) malloc(DESERT_MTU);
}
void nb__return_send_buffer(char* p) {
	free(p);
}
