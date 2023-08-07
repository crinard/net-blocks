#include <stdio.h>
#include <unistd.h>

#include "nb_runtime.h"

#define SERVER_MSG ("Hello from server")
using namespace nb1;

char client_id[] = {0, 0, 0, 0, 0, 2};
char server_id[] = {0, 0, 0, 0, 0, 1};

int running = 1;
static void callback(int event, nb__connection_t* c) {
  if (event == QUEUE_EVENT_READ_READY) {
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

  nb__connection_t* conn = nb__establish(client_id, 8081, 8080, callback);

  while (running) {
    nb__main_loop_step();
    usleep(100 * 1000);
  }
  nb__destablish(conn);
  return 0;
}