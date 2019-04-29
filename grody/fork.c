#include "fork.h"

#include <stdio.h>  //fprintf


static inline
void
client_handler(int sock, struct server *server)
{
    struct client* client;

    client = client_create(server, sock);

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
}


void
run_server_forked(struct server *server)
{
    int sock;
    struct sockaddr_in client_addr;
    int client_addr_len;
    int pid;

    client_addr_len = sizeof(struct sockaddr_in);

    while((sock = accept(server->sock, (struct sockaddr *)&client_addr, (socklen_t*)&client_addr_len))) {

        pid = fork();

        if (pid == 0) {
            // do child Processing
            close(server->sock);
            client_handler(sock, server);
            return;
        }
        else if (pid == -1) {
            printf("fork failed, may have run out of free handles\n");
            // maybe close all processes at this point?
            // block until more have left?
            // TODO: handle this better.
        }

        close(sock);
    }
}
