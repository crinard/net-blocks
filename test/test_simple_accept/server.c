#include "nb_runtime.h"
#include <unistd.h>
#include <stdio.h>

#define SERVER_MSG ("Hello from server")

char client_id[] = {0, 0, 0, 0, 0, 2};
char server_id[] = {0, 0, 0, 0, 0, 1};

int running = 1;
static void callback(int event, nb__connection_t * c) {
	if (event == QUEUE_EVENT_ACCEPT_READY) {
		// Share the callback function -- only event_accedt will fire with 2 tuple (wildcards) not 4 tuple.
		nb__connection_t * s = nb__accept(c, callback);
	} else if (event == QUEUE_EVENT_READ_READY) {	
		char buff[65];
		int len = nb__read(c, buff, 64);
		buff[len] = 0;	
		printf("Received = %s\n", buff);	
		nb__send(c, SERVER_MSG, sizeof(SERVER_MSG));
		running = 0;
	}
}

int main(int argc, char* argv[]) {
	nb__ipc_init("/tmp/ipc_socket", 1);
	printf("IPC initialized\n");
	nb__net_init();
	memcpy(nb__my_host_id, server_id, 6);
	// This specifies server id, etc. Sets netblockds identifier. Like ifconfig, setting address. We're looking for which "wire" to send on. For CSMA, we're creating a proxy, and specify headers in init function. 
	nb__connection_t * conn = nb__establish(nb__wildcard_host_identifier, 0, 8080, callback);
	// Every connection identified by 4 tuple, wildcard host identifier is used to listen on all interfaces, 0 is used to listen on any port, always sends on 8080, calls callback when new packet shows up. 
	while (running) {
		nb__main_loop_step();
		usleep(100 * 1000);
	}
	nb__destablish(conn);
	return 0;
}
