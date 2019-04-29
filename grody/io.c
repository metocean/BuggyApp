#include <sys/types.h>
#include <sys/socket.h>
#include <errno.h>
#include <stdio.h>


#include "webserver.h"


inline
int
client_read(struct client* client, char *buffer, size_t size, ssize_t *read)
{
    while(1) {
        *read = recv(client->sock, buffer, size, 0);

        if (*read == -1) {
            client->erron = errno;

            if (client->erron == EINTR) {
                continue;

            } else if (client->erron == EPIPE || client->erron == ECONNRESET) {
                client->error = client_disconnected_error;

            } else if (client->erron == EAGAIN) {
                client->error = client_read_timeout;

            } else {
                fprintf(stderr, "recv erron: %d, client: %d\n", client->erron, client->sock);
                client->error = request_reading_error;
            }

            return -1;

        } else if (*read == 0) {

            client->error = client_disconnected_error;
            return -1;
        }

        return 0;
    }
}


inline
int
client_read_until(struct client* client, char *buffer, ssize_t size)
{
    ssize_t read_s;

    while(size) {
        read_s = recv(client->sock, buffer, size, 0);

        if (read_s == -1) {
            client->erron = errno;

            if (client->erron == EINTR) {
                continue;

            } else if (client->erron == EPIPE || client->erron == ECONNRESET) {
                client->error = client_disconnected_error;

            } else if (client->erron == EAGAIN) {
                client->error = client_read_timeout;

            } else {
                fprintf(stderr, "recv erron: %d, client: %d\n", client->erron, client->sock);
                client->error = request_reading_error;
            }

            return -1;

        } else if (read_s == 0) {

            client->error = client_disconnected_error;
            return -1;
        }

        size -= read_s;
    }

    return 0;
}


inline
int
client_send(struct client* client, const void* buffer, size_t buffer_size)
{
    ssize_t writen;

    while (buffer_size > 0) {

        writen = send(client->sock, buffer, buffer_size, MSG_NOSIGNAL /*0*/ /*SERG*/);

        if (writen < 0) {
            client->erron = errno;

            if (client->erron == EINTR) {
                continue;

            } else if (client->erron == EPIPE || client->erron == ECONNRESET) {
                client->error = client_disconnected_error;

            } else if (client->erron == EAGAIN) {
                client->error = response_write_timeout;

            } else {
                client->error = response_writing_error;
                fprintf(stderr, "write erron: %d client: %d\n", client->erron, client->sock);
            }

            return -1;
        }

        buffer_size -= writen;
    }

    client->error = successful;
    return 0;
}
