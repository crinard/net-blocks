#ifndef NB_DESERT_IMPL_H
#define NB_DESERT_IMPL_H

#include "gen_headers.h"
#include "nb_timer.h"

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

char* nb__poll_packet(int*, int);
int nb__send_packet(char*, int);
void nb__ipc_init(const char* sock_path, int mode);
void nb__ipc_deinit();
void nb__desert_init(void);
char* nb__request_send_buffer(void);
void* nb__return_send_buffer(char*);

#ifdef __cplusplus
}
#endif // __cplusplus
#endif // NB_DESERT_IMPL_H