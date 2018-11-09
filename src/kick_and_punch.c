#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <pthread.h>
#include <ctype.h>

#include "debug.h"
#include "server.h"

#define port 9999
#define ip_address "127.0.0.1"
#define client_thread_size 50
#define client_listen_size 50

typedef struct
{
    int32_t base_sock_id;
    int32_t pair_sock_id;
    int32_t matched;
    int32_t active;
}player;

player players[client_thread_size];
pthread_mutex_t lock = PTHREAD_MUTEX_INITIALIZER;

/**
 * @brief For set player information and socket number
 *
 * @param For set sock_id number or clean sock_id
 *
 * @param If it is 0, it will clean sock_id number
 *        Or set sock_id number
 *
 * @return Error number
*/
int32_t search_empty_and_set_socket_number(int32_t sock_id, int32_t condition)
{
    int32_t i;
    pthread_mutex_lock(&lock);
    if(condition)
    {
        for(i = 0; i<client_thread_size ; i++)
        {
            if(players[i].active == 0)
            {
                players[i].base_sock_id = sock_id;
                players[i].pair_sock_id = 0;
                players[i].matched = 0;
                players[i].active = 1;
                #ifdef DEBUG
                    DEBUG_INFO("Set socket empty a place");
                #endif
                break;
            }
        }
    }
    else
    {
        for(i = 0; i<client_thread_size ; i++)
        {
            if(players[i].active == 1)
            {
                players[i].base_sock_id = 0;
                players[i].pair_sock_id = 0;
                players[i].matched = 0;
                players[i].active = 0;
                break;
            }
        }
    }
    pthread_mutex_unlock(&lock);
    #ifdef DEBUG
        DEBUG_INFO("Socket list id = %d", i);
    #endif
    return i;
}

/**
 * @brief It creates for each socket.
 *
 * @param Sock_id pointer
*/
void *socket_thread(void *arg)
{
    int32_t new_socket = *((int32_t *)arg);
    int32_t socket_list_id = search_empty_and_set_socket_number(new_socket,1);
    #ifdef DEBUG
        DEBUG_INFO("Socket list id = %d", socket_list_id);
    #endif
    player *new_player = &players[socket_list_id];
    int8_t client_message[1024];
    int32_t control = 0;

    strncpy(client_message, "Searching Player\n", 1024);
    if(send(new_player->base_sock_id, client_message, 1024, MSG_NOSIGNAL) < 0)
    {
        #ifdef DEBUG
            DEBUG_INFO("Didn't send data");
        #endif

        search_empty_and_set_socket_number(new_player->base_sock_id, 0);
        close(new_player->base_sock_id);
        control = 1;
        pthread_exit(NULL);
    }

    for (;;)
    {
        sleep(1);
        #ifdef DEBUG
            DEBUG_INFO("Sleeping sock_id=%d", new_player->base_sock_id);
        #endif
        if(new_player->matched == 1)
        {
            #ifdef DEBUG
                DEBUG_INFO("Matched");
            #endif
            for (;;)
            {
                memset(client_message, '\0', 1024);
                if(recv(new_player->base_sock_id, client_message, 1024, MSG_NOSIGNAL) <= 0)
                {
                    #ifdef DEBUG
                        DEBUG_INFO("Exit socket_thread \n");
                    #endif

                    search_empty_and_set_socket_number(new_player->base_sock_id, 0);
                    close(new_player->base_sock_id);
                    control = 1;
                    break;
                }

                // Kick
                if (!strcmp("1", client_message))
                {
                    #ifdef DEBUG
                        DEBUG_INFO("KICK, sock_id=%d", new_player->base_sock_id);
                    #endif
                    if(send(new_player->pair_sock_id, client_message, 1024, MSG_NOSIGNAL) < 0)
                    {
                        #ifdef DEBUG
                            DEBUG_INFO("Didn't send data");
                        #endif

                        search_empty_and_set_socket_number(new_player->base_sock_id, 0);
                        close(new_player->base_sock_id);
                        control = 1;
                        break;
                    }
                }
                // Punch
                else if(!strcmp("2", client_message))
                {
                    #ifdef DEBUG
                        DEBUG_INFO("PUNCH, sock_id=%d", new_player->base_sock_id);
                    #endif
                    if(send(new_player->pair_sock_id, client_message, 1024, MSG_NOSIGNAL) < 0)
                    {
                        #ifdef DEBUG
                            DEBUG_INFO("Didn't send data");
                        #endif

                        search_empty_and_set_socket_number(new_player->base_sock_id, 0);
                        close(new_player->base_sock_id);
                        control = 1;
                        break;
                    }
                }
                else
                {
                    #ifdef DEBUG
                        DEBUG_INFO("UNKNOWN Client Message : %s", client_message);
                    #endif
                    memset(client_message, '\0', 1024);
                    strncpy(client_message, "UNKNOWN Command", 1024);
                    if(send(new_player->base_sock_id, client_message, 1024, MSG_NOSIGNAL) < 0)
                    {
                        #ifdef DEBUG
                            DEBUG_INFO("Didn't send data");
                        #endif

                        search_empty_and_set_socket_number(new_player->base_sock_id, 0);
                        close(new_player->base_sock_id);
                        control = 1;
                        break;
                    }
                }
            }
        }
        if(control)
        {
            pthread_exit(NULL);
            break;
        }
    }
}

/*
    oyuncu arama islemini server yapacak
    client lar sadece uzak sunucudan veri gelmesini bekleyecek eslestirme gerceklestigine dair veri gonderecek
    client lar matched degiskenini kontrol edecek.
    eslesen client lar birbiri ile haberlesme icinde olacak.
*/

/**
 * @brief It does search player for game
*/
void *searching_player()
{
    int32_t i,j;
    for(;;)
    {
        for (i = 0; i < client_thread_size; i++)
        {
            if((players[i].matched == 0) && (players[i].active == 1))
            {
                for (j = i+1; j < client_thread_size; j++)
                {
                    if((players[j].matched == 0) && (players[j].active == 1))
                    {
                        players[i].pair_sock_id = players[j].base_sock_id;
                        players[j].pair_sock_id = players[i].base_sock_id;
                        players[i].matched = 1;
                        players[j].matched = 1;
                        #ifdef DEBUG
                            DEBUG_INFO("matched %d %d %d %d ", players[i].base_sock_id, players[j].base_sock_id, players[i].matched, players[j].matched);
                        #endif
                    }
                    j++;
                }
            }
            i++;
        }
        sleep(1);
        #ifdef DEBUG
            DEBUG_INFO("searching_player");
        #endif
    }
}

/**
 * @brief Main loop
*/
void start()
{
    int32_t server_socket, new_socket;
    struct sockaddr_in server_addr;
    struct sockaddr_storage server_storage;
    socklen_t addr_size;
    server_socket = socket(PF_INET, SOCK_STREAM, 0);
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    server_addr.sin_addr.s_addr = inet_addr(ip_address);
    memset(server_addr.sin_zero, '\0', sizeof server_addr.sin_zero);
    bind(server_socket, (struct sockaddr *) &server_addr, sizeof(server_addr));
    if(listen(server_socket,client_listen_size) == 0)
    {
        #ifdef DEBUG
            DEBUG_INFO("Listening");
        #endif
    }
    else
    {
        #ifdef DEBUG
            DEBUG_INFO("Error");
        #endif
    }

    pthread_t tid[client_thread_size];
    pthread_t search_thread;

    if(pthread_create(&search_thread, NULL, searching_player, NULL) != 0)
    {
        #ifdef DEBUG
            DEBUG_INFO("Search thread failed to create thread");
        #endif
    }

    int32_t i = 0;

    while(1)
    {
        addr_size = sizeof server_storage;
        new_socket = accept(server_socket, (struct sockaddr *) &server_storage, &addr_size);
        if (new_socket < 0)
        {
            #ifdef DEBUG
                DEBUG_INFO("Didn't create socket");
            #endif
        }
        else
        {
            if(i <= client_thread_size)
            {
                if(pthread_create(&tid[i], NULL, socket_thread, &new_socket) != 0)
                {
                    // close socket
                    close(new_socket);
                    #ifdef DEBUG
                        DEBUG_INFO("Failed to create thread");
                    #endif
                }
                else
                {
                    #ifdef DEBUG
                        DEBUG_INFO("Thread olusturuldu. Socket number = %d", new_socket);
                    #endif
                    i++;
                }
            }
        }

        if(i >= client_thread_size)
        {
            #ifdef DEBUG
                DEBUG_INFO("i >= client_thread_size");
            #endif
            i = 0;
            while(i < client_thread_size)
            {
                pthread_join(tid[i++],NULL);
            }
            i = 0;
        }
    }
}
