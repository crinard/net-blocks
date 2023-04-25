#include "nb_desert_impl.h"

char* nb__poll_packet(int*, int) {

}
int nb__send_packet(char*, int) {

}
void nb__ipc_init(const char* sock_path, int mode) {
    ipc_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (ipc_socket < 0) {
		fprintf(stderr, "Socket create failed\n");
		exit(-1);
	}

	if (mode == 0) {
		// connect mode
		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, sock_path);
		if (connect(ipc_socket, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) < 0) {
			fprintf(stderr, "Socket connect failed\n");
			exit(-1);
		}
		fcntl(ipc_socket, F_SETFL, O_NONBLOCK);
	} else {
		// Delete any stale sockets
		unlink(sock_path);
		// Bind mode
		struct sockaddr_un addr;
		int ipc_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
		if (ipc_fd < 0) {
			fprintf(stderr, "Socket create failed\n");
			exit(-1);
		}
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, sock_path);
		if (bind(ipc_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			fprintf(stderr, "Socket bind failed\n");
			exit(-1);
		}
		listen(ipc_fd, 1);
		ipc_socket = accept(ipc_fd, NULL, NULL);
		if (ipc_socket < 0) {
			fprintf(stderr, "Socket accept failed\n");
			exit(-1);
		}
		fcntl(ipc_socket, F_SETFL, O_NONBLOCK);
		close(ipc_fd);
	}
}
void nb__ipc_deinit() {
    ipc_socket = socket(AF_UNIX, SOCK_SEQPACKET, 0);
	if (ipc_socket < 0) {
		fprintf(stderr, "Socket create failed\n");
		exit(-1);
	}

	if (mode == 0) {
		// connect mode
		struct sockaddr_un addr;
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, sock_path);
		if (connect(ipc_socket, (struct sockaddr*) &addr, sizeof(struct sockaddr_un)) < 0) {
			fprintf(stderr, "Socket connect failed\n");
			exit(-1);
		}
		fcntl(ipc_socket, F_SETFL, O_NONBLOCK);
	} else {
		// Delete any stale sockets
		unlink(sock_path);
		// Bind mode
		struct sockaddr_un addr;
		int ipc_fd = socket(AF_UNIX, SOCK_SEQPACKET, 0);
		if (ipc_fd < 0) {
			fprintf(stderr, "Socket create failed\n");
			exit(-1);
		}
		memset(&addr, 0, sizeof(addr));
		addr.sun_family = AF_UNIX;
		strcpy(addr.sun_path, sock_path);
		if (bind(ipc_fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
			fprintf(stderr, "Socket bind failed\n");
			exit(-1);
		}
		listen(ipc_fd, 1);
		ipc_socket = accept(ipc_fd, NULL, NULL);
		if (ipc_socket < 0) {
			fprintf(stderr, "Socket accept failed\n");
			exit(-1);
		}
		fcntl(ipc_socket, F_SETFL, O_NONBLOCK);
		close(ipc_fd);
	}
}
void nb__desert_init(void) {

}
char* nb__request_send_buffer(void) {

}
void* nb__return_send_buffer(char*) {

}