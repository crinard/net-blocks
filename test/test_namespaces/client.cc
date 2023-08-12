#include <stdio.h>
#include <unistd.h>

#include "nb_runtime.h"
#define CLIENT_MSG ("Hello from client")
char client_id[] = {0, 0, 0, 0, 0, 2};
char server_id[] = {0, 0, 0, 0, 0, 1};

int running = 1;
static void nb1callback(int event, nb1::nb__connection_t* c) {
  if (event == QUEUE_EVENT_READ_READY) {
    char buff[65];
    int len = nb1::nb__read(c, buff, 64);
    buff[len] = 0;
    printf("Received = %s\n", buff);
    running = 0;
  }
}

static void nb2callback(int event, nb2::nb__connection_t* c) {
  if (event == QUEUE_EVENT_READ_READY) {
    char buff[65];
    int len = nb2::nb__read(c, buff, 64);
    buff[len] = 0;
    printf("Received = %s\n", buff);
    running = 0;
  }
}

int main(int argc, char* argv[]) {
  nb1::nb__mix_init();
  nb2::nb__mix_init();
  printf("MIX initialized\n");

  nb1::nb__net_init();
  nb2::nb__net_init();
  printf("NET initialized\n");

  memcpy(nb1::nb__my_host_id, client_id, 6);
  memcpy(nb2::nb__my_host_id, server_id, 6);

  nb1::nb__connection_t* conn1 =
      nb__establish(server_id, 8080, 8081, nb1callback);
  nb2::nb__connection_t* conn2 =
      nb__establish(server_id, 8080, 8081, nb2callback);

  nb1::nb__send(conn1, CLIENT_MSG, sizeof(CLIENT_MSG));

  while (running) {
    nb1::nb__main_loop_step();
    nb2::nb__main_loop_step();
    usleep(100 * 1000);
  }
  nb1::nb__destablish(conn1);
  nb2::nb__destablish(conn2);
  return 0;
}
