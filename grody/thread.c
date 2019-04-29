#include "thread.h"

#include <stdio.h>  //fprintf
#include <pthread.h>
#include <stdlib.h>


static inline
void *
client_handler(void *arg)
{
    struct client *client;
    client  = (struct client*) arg;

    pthread_cleanup_push(client_free, client);

    if (client->_server->on_connection_created)
        client->_server->on_connection_created(client);

    if (-1 == setsockopt(client->sock, SOL_SOCKET, SO_LINGER,
        &client->_server->client_linger,
        sizeof(client->_server->client_linger)))
        fprintf(stderr, "failed to set client(%d) SO_LINGER", client->sock);

    if (-1 == setsockopt(client->sock, SOL_SOCKET, SO_RCVTIMEO,
            (const char*)&client->_server->client_read_timeout,
            sizeof(client->_server->client_read_timeout)))
        fprintf(stderr, "failed to set client(%d) SO_RCVTIMEO", client->sock);

    if (-1 == setsockopt(client->sock, SOL_SOCKET, SO_SNDTIMEO,
            (const char*)&client->_server->client_write_timeout,
            sizeof(client->_server->client_write_timeout)))
        fprintf(stderr, "failed to set client(%d) SO_SNDTIMEO", client->sock);

    client_loop(client);

    pthread_cleanup_pop(1);
    return 0;
}


void
run_server_threaded(struct server *server)
{
    int sock;
    struct sockaddr_in client_addr;
    int client_addr_len;
    struct client *client;
    pthread_t thread_id;
    pthread_attr_t thread_attr;

    pthread_attr_init(&thread_attr);
    pthread_attr_setdetachstate(&thread_attr, 1);

    client_addr_len = sizeof(struct sockaddr_in);

    while((sock = accept(server->sock, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len))) {

        client = client_create(server, sock);

        if(pthread_create(&thread_id, &thread_attr, client_handler , client) < 0) {

            client_free(client);
            perror("could not create thread");
        }
    }

    fprintf(stderr, "server stopped listening on port:%d\n", server->port);
}

static inline
void *
client_1_thread_handler(void *arg)
{
    struct client *client;
    client  = (struct client*) arg;

    

    if (client->_server->on_connection_created)
        client->_server->on_connection_created(client);

    if (-1 == setsockopt(client->sock, SOL_SOCKET, SO_LINGER,
        &client->_server->client_linger,
        sizeof(client->_server->client_linger)))
        fprintf(stderr, "failed to set client(%d) SO_LINGER", client->sock);

    if (-1 == setsockopt(client->sock, SOL_SOCKET, SO_RCVTIMEO,
            (const char*)&client->_server->client_read_timeout,
            sizeof(client->_server->client_read_timeout)))
        fprintf(stderr, "failed to set client(%d) SO_RCVTIMEO", client->sock);

    if (-1 == setsockopt(client->sock, SOL_SOCKET, SO_SNDTIMEO,
            (const char*)&client->_server->client_write_timeout,
            sizeof(client->_server->client_write_timeout)))
        fprintf(stderr, "failed to set client(%d) SO_SNDTIMEO", client->sock);

    client_loop(client);

    client_free(client);

    return 0;
}


void
run_server_1_thread(struct server *server)
{
    int sock;
    struct sockaddr_in client_addr;
    int client_addr_len;
    struct client *client;

    client_addr_len = sizeof(struct sockaddr_in);

    while((sock = accept(server->sock, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len))) {

        client = client_create(server, sock);

        client_handler(client);
    }

    fprintf(stderr, "server stopped listening on port:%d\n", server->port);
}
