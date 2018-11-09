#ifndef SERVER_H
#define SERVER_H

#include <stdint.h>

int32_t search_empty_and_set_socket_number(int32_t sock_id, int32_t condition);
void *socket_thread(void *arg);
void *searching_player();
void start();

#endif
