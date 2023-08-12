/*
IPC + DESERT runtime mix to run desert code outside of ns2
*/
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include <iostream>
#include <queue>
#include <utility>
#include <vector>

#define IPC_MTU 1024

static char* tto_sent_data = NULL;
static int tto_sent_data_len = 0;
static bool tto_busy = false;
static char* ott_sent_data = NULL;
static int ott_sent_data_len = 0;
static bool ott_busy = false;

static std::queue<std::pair<char*, int>> tto_queue;
static std::queue<std::pair<char*, int>> ott_queue;
namespace nb1 {
static char out_of_order_store[IPC_MTU];

static int out_of_order_len = 0;
static int nb1tx_pkts = 0;
static size_t nb1tx_bytes = 0;
static int nb2tx_pkts = 0;
static size_t nb2tx_bytes = 0;

void nb__mix_init(void) { return; }

void nb__mix_deinit(void) {
  // fprintf(stdout,
  //         "Finish, nb nb_ll_p_tx = %i, nb_ll_b_tx = %lu, nb_ll_p_rx = %i, "
  //         "nb_ll_b_rx = %lu\n",
  //         nb_ll_p_tx, nb_ll_b_tx, nb_ll_p_rx, nb_ll_b_rx);
  return;
}

char nb__reuse_mtu_buffer[IPC_MTU];

/**
 * @brief polls packets from those read into nb_read_pkt_buf from the recv()
 * function in nb_p.c
 *
 * @param size
 * @param headroom
 * @return char*
 */
char* nb__poll_packet(int* size, int headroom) {
  if (tto_busy) {
    char* ret = (char*)malloc(IPC_MTU + headroom);
    *size = tto_sent_data_len;
    memcpy(ret + headroom, tto_sent_data, tto_sent_data_len);
    tto_busy = false;
    free(tto_sent_data);
    return ret;
  }
  return NULL;
}

static int uidcnt_ = 0;
static int send_cnt = 0;
/**
 * @brief Sends a packet down a layer
 *
 * @param buff Packet to send
 * @param len
 * @return int 0 or 1 always
 */
int nb__send_packet(char* buff, int len) {
  char* tmp = (char*)malloc(len);
  memcpy(tmp, buff, len);
  ott_queue.push(std::make_pair(tmp, len));
  // Add to queue & return
  if (ott_busy) return 0;
  ott_busy = true;
  std::pair<char*, int> pa = ott_queue.front();
  ott_queue.pop();
  ott_sent_data = pa.first;
  int l = pa.second;
  ott_sent_data_len = l;
  return 0;
}
}  // namespace nb1

namespace nb2 {
void nb__mix_init(void) { return; }

void nb__mix_deinit(void) {
  // fprintf(stdout,
  //         "Finish, nb nb_ll_p_tx = %i, nb_ll_b_tx = %lu, nb_ll_p_rx = %i, "
  //         "nb_ll_b_rx = %lu\n",
  //         nb_ll_p_tx, nb_ll_b_tx, nb_ll_p_rx, nb_ll_b_rx);
  return;
}

char nb__reuse_mtu_buffer[IPC_MTU];

/**
 * @brief polls packets from those read into nb_read_pkt_buf from the recv()
 * function in nb_p.c
 *
 * @param size
 * @param headroom
 * @return char*
 */
char* nb__poll_packet(int* size, int headroom) {
  if (ott_busy) {
    char* ret = (char*)malloc(IPC_MTU + headroom);
    *size = ott_sent_data_len;
    memcpy(ret + headroom, ott_sent_data, ott_sent_data_len);
    ott_busy = false;
    free(ott_sent_data);
    return ret;
  }
  return NULL;
}

static int uidcnt_ = 0;
static int send_cnt = 0;
/**
 * @brief Sends a packet down a layer
 *
 * @param buff Packet to send
 * @param len
 * @return int 0 or 1 always
 */
int nb__send_packet(char* buff, int len) {
  char* tmp = (char*)malloc(len);
  memcpy(tmp, buff, len);
  tto_queue.push(std::make_pair(tmp, len));
  // Add to queue & return
  if (tto_busy) return 0;
  tto_busy = true;
  std::pair<char*, int> pa = tto_queue.front();
  tto_queue.pop();
  tto_sent_data = pa.first;
  int l = pa.second;
  tto_sent_data_len = l;
  return 0;
}

char* nb__request_send_buffer(void) { return (char*)malloc(IPC_MTU); }
void nb__return_send_buffer(char* p) { free(p); }
}  // namespace nb2