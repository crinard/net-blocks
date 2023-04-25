#include "nb_runtime.h"
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <packet.h>
#include <module.h>

#include "nb_p.h"

static const Nb_pModule *m;

#define IPC_MTU (1024)
int nb__ipc_simulate_out_of_order = 0;
int nb__ipc_simulate_packet_drop = 0;
static char out_of_order_store[IPC_MTU];
static int out_of_order_len = 0;
#define OUT_OF_ORDER_CHANCE (5)
#define PACKET_DROP_CHANCE (5)

void nb__ipc_init(Nb_pModule *_m) {
	// connect mode
	m = _m;
	return;
}

void nb__ipc_deinit(void) {
	return;
}

char nb__reuse_mtu_buffer[IPC_MTU];

/**
 * @brief polls packets from those read into nb_read_pkt_buf from the recv() function in nb_p.c
 * 
 * @param size 
 * @param headroom 
 * @return char* 
 */
char* nb__poll_packet(int* size, int headroom) {
	int len;
	static char temp_buf[IPC_MTU];
	vector<Packet*> readbuf = m->getNBReadBuffer();
	*size = 0;//TODO: Should assert?
	char ** ret = malloc(sizeof(char*) * readbuf.size());
	for (int i = 0; i < readbuf.size(); i++) {
		Packet* p = readbuf[i];
		unsigned char* data = p->data();
		//TODO: how to handle multiple packets from different sources?
		if (p->size() > 0) {
			char* buf = malloc(IPC_MTU + headroom);
			memcpy(buf + headroom, data , IPC_MTU);
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
	// Don't try to out of order if we already have on pending
	if (out_of_order_len == 0 && nb__ipc_simulate_out_of_order) {
		int r = rand() % OUT_OF_ORDER_CHANCE;
		if (r == 0) {
			out_of_order_len = len;
			memcpy(out_of_order_store, buff, len);
			return len;
		}
	}
	//nb__debug_packet(buff);
	Packet* p = Packet::new();

	p->data() = buff;
	p->size() = len;
	//TODO: IP headers, or generically writing -- need to ask Ajay.
	int ret = m->sendDown(p,0);
	// If there is a pending packet, send it now
	if (out_of_order_len) {
		Packet* out_of_order_p = Packet::new();
		out_of_order_p->data() = out_of_order_store;
		out_of_order_p->size() = out_of_order_len;
		int ret = m->sendDown(p, 0);
		out_of_order_len = 0;
	}
	return ret;
}


char* nb__request_send_buffer(void) {
	return malloc(IPC_MTU);
}
void* nb__return_send_buffer(char* p) {
	free(p);
}
