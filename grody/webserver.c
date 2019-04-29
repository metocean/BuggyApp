#include "webserver.h"
#include "fork.h"
#include "thread.h"

#include <stdio.h>  //fprintf
#include <stdlib.h> //malloc

#include <unistd.h>
#include <netinet/tcp.h>
#include <errno.h>
#include <string.h>
#include <netdb.h>
#include <stdarg.h>
#include <inttypes.h>
#include <assert.h>
#include <signal.h>

const char* mime_text = "text/plain";
const char* mime_json = "application/json";
const char* mime_binary = "application/octet-stream";
const char* mime_png = "image/png";
const char* mime_jpeg = "image/jpeg";

const char* http_continue_text = "Continue";
const char* http_switching_protocols_text = "Switching Protocols" ;
const char* http_processing_text = "Processing";
const char* http_early_hint_text = "Early Hint";

const char* http_ok_text = "OK" ;
const char* http_created_text = "Created" ;
const char* http_accepted_text = "Accepted" ;
const char* http_unauthorized_text = "Non-Authoritative Information" ;
const char* http_no_content_text = "No Content" ;
const char* http_reset_content_text = "Reset Content" ;
const char* http_partial_content_text = "Partial Content" ;

const char* http_multiple_choices_text = "Multiple Choices";
const char* http_moved_permanently_text = "Moved Permanently";
const char* http_found_text = "Found";
const char* http_see_other_text = "See Other";
const char* http_not_modified_text = "Not Modified";
const char* http_use_proxy_text = "Use Proxy";
const char* http_switch_proxy_text = "Switch Proxy";
const char* http_temporary_redirect_text = "Temporary Redirect";
const char* http_permanent_redirect_text = "Permanent Redirect";

const char* http_internal_server_error_text = "Internal Server Error";
const char* http_not_implemented_text = "Not Implemented";
const char* http_bad_gateway_text = "Not Bad Gateway";
const char* http_service_unavailable_text = "Service Unavailable";
const char* http_gateway_timeout_text = "Gateway Timeout";
const char* http_version_not_supported_text = "HTTP Version Not Supported";
const char* http_network_auth_required_text = "Network Authentication Required";


static const char *http_header_format =  "HTTP/1.1 %d %s\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %d\r\n"
    "\r\n";


static const char *http_header_format_keep_alive =  "HTTP/1.1 %d %s\r\n"
    "Connection: keep-alive\r\n"
    "Content-Type: %s\r\n"
    "Content-Length: %d\r\n"
    "\r\n";


static const char *http_header_not_found = "HTTP/1.1 404 Not Found\r\n"
    "Content-Length: 0\r\n"
    "\r\n";


static const char *http_header_not_found_keep_alive = "HTTP/1.1 404 Not Found\r\n"
    "Connection: keep-alive\r\n"
    "Content-Length: 0\r\n"
    "\r\n";


static const char *http_header_header_only_format = "HTTP/1.1 %d %s\r\n"
    "Content-Length: 0\r\n"
    "\r\n";


static const char *http_header_header_only_format_keep_alive = "HTTP/1.1 %d %s\r\n"
    "Connection: keep-alive\r\n"
    "Content-Length: 0\r\n"
    "\r\n";


const int default_port = 8080;
const int default_accept_backlog = 100;
const int default_client_read_timeout = 5;
const int default_client_write_timeout = 10;
const int default_client_socket_linger = 10;
const size_t default_client_buffer_size = 1024 * 8;
const size_t default_client_body_max_size = 1024 * 128;
const size_t default_client_body_increment_size = 1024 * 8;


void
server_settings_defaults(struct server_settings* settings)
{
    memset(settings, 0, sizeof(struct server_settings));

    settings->port = default_port;
    settings->accept_backlog = default_accept_backlog;
    settings->client_read_timeout = default_client_read_timeout;
    settings->client_write_timeout = default_client_write_timeout;
    settings->client_socket_linger = default_client_socket_linger;
    settings->client_buffer_size = default_client_buffer_size;
    settings->client_body_max_size = default_client_body_max_size;
    settings->client_body_increment_size = default_client_body_increment_size;
}


inline
void
client_shutdown(struct client* client)
{
    client->_keep_alive = 0;
}


inline
void
client_disconnect(struct client* client)
{
    client->_keep_alive = 0;
    client->error = force_disconnect_error;
}


inline
int
not_found_response(struct client *client)
{
    if (client->_keep_alive && client->req.minor_version == 0)
        return client_send(client, http_header_not_found_keep_alive, strlen(http_header_not_found_keep_alive));
    return client_send(client, http_header_not_found, strlen(http_header_not_found));
}


inline
int
start_response(struct client *client,
    int status, const char* status_text,
    const char *content_type, size_t content_length)
{
    char buffer[1024 * 4];
    size_t buffers_size;

    if (client->_keep_alive && client->req.minor_version == 0) {

        if (content_type == NULL) {
            buffers_size = snprintf(buffer,
                     sizeof(buffer),
                     http_header_header_only_format_keep_alive,
                     status,
                     status_text);

        } else {

            buffers_size = snprintf(buffer,
                     sizeof(buffer),
                     http_header_format_keep_alive,
                     status,
                     status_text,
                     content_type,
                     content_length);
        }
    } else {

        if (content_type == NULL) {
            buffers_size = snprintf(buffer,
                     sizeof(buffer),
                     http_header_header_only_format,
                     status,
                     status_text);

        } else {

            buffers_size = snprintf(buffer,
                     sizeof(buffer),
                     http_header_format,
                     status,
                     status_text,
                     content_type,
                     content_length);
        }
    }

    return client_send(client, buffer, buffers_size);
}


inline
int
respond(struct client *client, int status, const char* status_text,
         const char *content_type, const void *content, size_t content_length)
{
    if (content_type == NULL)
        return start_response(client, status, status_text, NULL, 0);

    if (start_response(client, status, status_text, content_type, content_length))
        return -1;

    return client_send(client, content, content_length);
}


// used for sending error message to the user, it will shutdown the client's
// socket when finished.
static inline
void
respond_and_shutdown(struct client *client, int status, const char* status_text)
{
    int no_delay;
    no_delay = 1;

    respond(client, status, status_text, NULL, NULL, 0);
    // flush socket
    setsockopt(client->sock, IPPROTO_TCP, TCP_NODELAY, (char *) &no_delay, sizeof(int));
    // shutdown client.
    shutdown(client->sock, SHUT_RDWR);
}


inline
int
respondf(struct client *client,
        int status,
        const char* status_text,
        const char *content_type,
        const char *format, ...)
{
    int length;
    char *content;
    va_list args;
    va_start(args, format);
    length = vasprintf(&content, format, args);
    va_end(args);

    if (length == -1)
        return -1;

    if (start_response(client, status, status_text, content_type, length)) {
        free(content);
        return -2;
    }

    if (client_send(client, content, length)) {
        free(content);
        return -3;
    }

    free(content);
    return 0;
}


static inline
int
read_request_header(struct client* client)
{
    ssize_t s_read, prev_buffer_size;
    struct request *req;

    req = &client->req;

    // set max number of headers.
    req->num_headers = sizeof(req->headers) / sizeof(req->headers[0]);

    // null terminate end of read buffer.
    req->_buffer_used = 0;

    while (1) {

        if (client_read(client, &req->_buffer[req->_buffer_used],
            req->_buffer_size - req->_buffer_used, &s_read))
          return -1;

        prev_buffer_size = req->_buffer_used;
        req->_buffer_used += s_read;

        req->header_size = phr_parse_request(req->_buffer, req->_buffer_used,
                &req->method, &req->method_len, &req->url, &req->url_len,
                &req->minor_version, req->headers, &req->num_headers,
                prev_buffer_size);

        if (req->header_size == -2 && req->_buffer_used == req->_buffer_size) {
            client->error = request_to_big_error;
            return -1;

        } else if (req->header_size > 0) {
            client->error = successful;
            return 0;

        } else if (req->header_size == -1) {
            client->error = request_parsing_error;
            return -1;
        }
    }

    client->error = request_parsing_error;
    return -1;
}


inline static
int
is_header(const char *name, struct phr_header * header)
{
    return header->name_len == strlen(name)
            && strncasecmp(name, header->name, header->name_len) == 0;
}


// first string must be null terminated, 2nd string must supply a length.
inline static
int
strings_equal(const char *value1, const char *value2, size_t value2_len)
{
    return strlen(value1) == value2_len
            && strncasecmp(value1, value2, value2_len) == 0;
}


static
void
null_str(const char *str, size_t len)
{
    char *v;
    if (len <= 0)
        return;

    v = (char *)&str[len];
    *v = 0;
}


inline static
int
parse_request_header(struct client *client)
{
    struct request *req;
    struct phr_header * header;
    size_t i;

    req = &client->req;

    null_str(req->method, req->method_len);
    null_str(req->url, req->url_len);

    // parse parameter
    req->param = memchr(req->url, '?', req->url_len);
    // need bounds check!!
    if (req->param != NULL) {
        req->url_len = req->param - req->url;
        *((char*)req->param) = 0;
        req->param++;
        req->param_len = strlen(req->param);
    }

    // parse headers.
    req->content_length = -1;

    req->connection = NULL;
    req->connection_len = 0;

    req->content_type = NULL;
    req->content_type_len = 0;

    req->keep_alive = NULL;
    req->keep_alive_len = 0;

    req->transfer_encoding = NULL;
    req->transfer_encoding_len = 0;

    req->is_chunked_encoded = false;

    const char *content_length = NULL;

    for (i = 0; i < req->num_headers; i++) {
        header = &req->headers[i];

        // dirty hack to null header strings.
        null_str(header->name, header->name_len);
        null_str(header->value, header->value_len);

        if (req->connection == NULL && is_header("connection", header)) {
            req->connection = header->value;
            req->connection_len = header->value_len;

        } else if (content_length == NULL && is_header("content-length", header)) {
            char *end;
            req->content_length = strtoumax(header->value, &end, 10);

        } else if (req->content_type == NULL && is_header("content-type", header)) {
            req->content_type = header->value;
            req->content_type_len = header->value_len;

        } else if (req->keep_alive == NULL && is_header("keep-alive", header)) {
            req->keep_alive = header->value;
            req->keep_alive_len = header->value_len;

        } else if (req->keep_alive == NULL && is_header("transfer-encoding", header)) {
            req->transfer_encoding = header->value;
            req->transfer_encoding_len = header->value_len;

            req->is_chunked_encoded = strings_equal("chunked", req->transfer_encoding, req->transfer_encoding_len);
        }
    }

    return 0;
}


static inline
void
body_cleanup(struct client *client)
{
    struct request *req;
    req = &client->req;

    if (req->_body != NULL) {
        free(req->_body);
        req->_body = NULL;
    }

    client->req.body_size = 0;
    client->req.body = NULL;
}


inline static
int
read_body_by_content_lenght(struct client *client)
{
    ssize_t body_in_buffer;
    struct request *req;

    req = &client->req;

    body_in_buffer  = req->_buffer_used - req->header_size;

    if (body_in_buffer == req->content_length) {
        req->body = &req->_buffer[req->header_size];
        return 0;
    }

    req->_body = malloc(req->content_length);
    req->body = req->_body;

    if (req->_body == NULL) {
        fprintf(stderr, "failed to allocate memory for client body buffer sock:%d", client->sock);
        client->error = out_of_memory_error;
        return -1;
    }

    if (body_in_buffer) {
        memcpy(req->_body, &req->_buffer[req->header_size], body_in_buffer);
        return client_read_until(client, req->_body + body_in_buffer, req->content_length - body_in_buffer);

    } else {
        return client_read_until(client, req->_body, req->content_length);
    }
}


inline static
int
read_body_by_chunk(struct client *client)
{
    size_t size, capacity, increment_size, max_size;
    ssize_t body_in_buffer, ret, rsize;
    struct phr_chunked_decoder *decoder;
    struct request *req;

    req = &client->req;

    body_in_buffer  = req->_buffer_used - req->header_size;

    decoder = &req->chunk_decoder;
    memset(decoder, 0, sizeof(struct phr_chunked_decoder));
    // set consume_trailer to 1 to discard the trailing header
    decoder->consume_trailer = 1;

    increment_size = client->_server->client_body_increment_size;
    max_size = client->_server->client_body_max_size;
    capacity = body_in_buffer + increment_size;
    size = 0;

    if (body_in_buffer) {
        rsize = body_in_buffer;
        ret = phr_decode_chunked(decoder, &req->_buffer[req->header_size], (size_t*)rsize);

        if (ret == -1) {
            client->error = request_transfer_decoding_error;
            return -1;

        } if (ret != -2) {
            req->body = &req->_buffer[req->header_size];
            req->body_size = rsize;
            return 0;
        }
        size += rsize;

        req->_body = malloc(capacity);
        if (req->_body == NULL) {
            client->error = out_of_memory_error;
            return -1;
        }
        memcpy(req->_body, &req->_buffer[req->header_size], body_in_buffer);

    } else {

        req->_body = malloc(capacity);
        if (req->_body == NULL) {
            client->error = out_of_memory_error;
            return -1;
        }
    }

    req->has_more_body = false;
    do {
        // expand the buffer if necessary
        if (size == capacity) {
            if (size == max_size) {
                req->has_more_body = true;
                break;
            }

            capacity += increment_size;
            req->_body = realloc(req->_body, capacity);
            if (req->_body == NULL) {
                client->error = out_of_memory_error;
                return -1;
            }
        }

        if (client_read(client, req->_body + size, capacity - size, &rsize))
            return -1;

        ret = phr_decode_chunked(decoder, req->_body + size, (size_t*)&rsize);
        if (ret == -1) {
            client->error = request_transfer_decoding_error;
            return -1;
        }

        size += rsize;

    } while (ret == -2);

    req->body = req->_body;
    req->body_size = size;

    return 0;
}


inline static
int
read_body(struct client *client)
{
    if (client->req.content_length > 0 && client->req.is_chunked_encoded) {
        client->error = request_content_len_and_transfer_encoding_error;
        return -1;
    }

    if (client->req.content_length > 0)
        return read_body_by_content_lenght(client);

    else if (client->req.is_chunked_encoded)
        return read_body_by_chunk(client);

    return 0;
}


static inline
struct request_handler *
find_request_handler(struct client *client)
{
    size_t handler_uri_len;
    struct request *req;
    struct request_handler *request_handlers;

    req = &client->req;
    request_handlers = client->_server->request_handlers;

    for (int i = 0; request_handlers[i].handler; i++) {

        handler_uri_len = strlen(request_handlers[i].uri);

        if (handler_uri_len > req->url_len)
            continue;

        if (strncmp(request_handlers[i].uri, req->url, handler_uri_len) == 0)
            return &request_handlers[i];
    }

    return NULL;
}


inline static
int
is_keep_alive(struct client *client)
{
    struct request *req;
    req = &client->req;

    if (req->connection_len) {

        if  (strings_equal("close", req->connection, req->connection_len))
            return 0;

        else if (strings_equal("keep-alive", req->connection, req->connection_len))
            return 1;
    }

    // if http 1.1 it's assumed to keep alive, if 1.0 its not.
    return req->minor_version;
}


static inline
void
handle_client_error(struct client *client)
{
    switch (client->error) {
        case successful:
            break;

        case request_to_big_error:
            respond_and_shutdown(client, http_request_entity_too_large,
                    "request to large");
            break;

        case request_content_len_and_transfer_encoding_error:
            respond_and_shutdown(client, http_bad_request,
                    "cannot have content length and transfer encoding together");
            break;

        case out_of_memory_error:
            respond_and_shutdown(client, http_service_unavailable,
                    "server is out of memory, please try again soon");
            break;

        case not_implemented:
            respond_and_shutdown(client, http_not_implemented,
                    "sorry not implemented");
            break;

        case request_parsing_error:
            respond_and_shutdown(client, http_bad_request,
                    "bad request supplied");
            break;

        case request_transfer_decoding_error:
            respond_and_shutdown(client, http_bad_request,
                "invalid transfer encoding");
            break;

        // do nothing with the flowing errors as the client is most likely
        // disconnected.
        case client_disconnected_error:
        case client_read_timeout:
        case request_reading_error:
        case response_write_timeout:
        case response_writing_error:
        case force_disconnect_error:
            break;
    }
}


struct client *
client_create(const struct server *server, int sock)
{
    struct client *client;

    if ((client = calloc(1, sizeof(struct client))) == NULL)
      return NULL;

    client->_server = server;
    client->sock = sock;
    client->data_size = -1;

    if ((client->req._buffer = malloc(server->client_buffer_size)) == NULL) {
        free(client);
        return NULL;
    }

    client->req._buffer_size = server->client_buffer_size;

    return client;
}


void
client_free(void *arg)
{
    struct client *client;
    client = arg;

    if (arg == NULL)
      return;

    if (client->_server->on_connection_destruction)
        client->_server->on_connection_destruction(client);

    if (client->sock != -1) {
        close(client->sock);
        client->sock = -1;
    }

    if (client->req._buffer != NULL) {
        free(client->req._buffer);
        client->req._buffer = NULL;
    }

    // this cleanup could be called twice if the thread is cancelled, which
    // we will try and avoid at all costs.
    if (client->req._body != NULL) {
        free(client->req._body);
        client->req._body = NULL;
    }

    free(client);
}


void
client_loop(struct client* client)
{
    struct request_handler *req_handler;
    int no_delay;

    do {
        if (read_request_header(client))
            break;

        if (parse_request_header(client))
            break;

        if (read_body(client))
            break;

        client->_keep_alive = is_keep_alive(client);

        req_handler = find_request_handler(client);

        // allow TCP Nagle algorithm to collect small outgoing packets into bigger TCP packets.
        no_delay = 0;
        setsockopt(client->sock, IPPROTO_TCP, TCP_NODELAY, (char *) &no_delay, sizeof(int));

        if (req_handler == NULL)
            not_found_response(client);
        else
            req_handler->handler(client);

        if (client->error) {
            handle_client_error(client);
            break;
        }

        // flush write buffer.
        no_delay = 1;
        setsockopt(client->sock, IPPROTO_TCP, TCP_NODELAY, (char *) &no_delay, sizeof(int));

        body_cleanup(client);

        if (client->_keep_alive)
            continue;

        shutdown(client->sock, SHUT_RDWR);
        break;

    } while(1);
}


static
enum webserver_fatal_errors
start_listening(struct server *server)
{
    server->sock = socket(AF_INET , SOCK_STREAM , IPPROTO_TCP);
    if (server->sock == -1) {
        perror("could not create listener socket");
        return listener_socket_error;
    }

    int setopt = 1;
    if (-1 == setsockopt(server->sock, SOL_SOCKET, SO_REUSEADDR, (char*)&setopt, sizeof(setopt))) {
        perror("setting listener socket options");
        return listener_socket_options_error;
    }

    server->addr.sin_family = AF_INET;
    server->addr.sin_addr.s_addr = INADDR_ANY;
    server->addr.sin_port = htons(server->port);
    server->addr_len = sizeof(struct sockaddr_in);

    if (bind(server->sock, (struct sockaddr *)&server->addr , sizeof(server->addr)) < 0) {
        //print the error message
        perror("bind failed. Error");
        return listener_bind_error;
    }

    if (-1 == listen(server->sock , server->accept_backlog)) {
        perror("listen failed");
        return listener_listen_error;
    }

    return successful;
}


void
server_init(
    struct server* server,
    struct server_settings* settings,
    struct request_handler* handlers,
    on_connection_created_cb on_connection_created,
    on_connection_destruction_cb on_connection_destruction)
{
    memset(server, 0, sizeof(struct server));

    server->port = settings->port > 0 ? settings->port : default_port;
    server->accept_backlog = settings->accept_backlog > 0 ? settings->accept_backlog : default_accept_backlog;

    server->on_connection_created = on_connection_created;
    server->on_connection_destruction = on_connection_destruction;

    server->client_read_timeout.tv_usec = 0;
    server->client_read_timeout.tv_sec = settings->client_read_timeout > 0 ? settings->client_read_timeout : default_client_read_timeout;

    server->client_write_timeout.tv_usec = 0;
    server->client_write_timeout.tv_sec = settings->client_write_timeout > 0 ? settings->client_write_timeout : default_client_write_timeout;

    server->client_linger.l_onoff = 1;
    server->client_linger.l_linger = settings->client_socket_linger > 0 ? settings->client_socket_linger : default_client_socket_linger;

    server->client_buffer_size = settings->client_buffer_size > 0 ? settings->client_buffer_size : default_client_buffer_size;
    server->client_body_max_size = settings->client_body_max_size > 0 ? settings->client_body_max_size : default_client_body_max_size;
    server->client_body_increment_size = settings->client_body_increment_size > 0 ? settings->client_body_increment_size : default_client_body_increment_size;

    server->request_handlers = handlers;
}


enum webserver_fatal_errors
run_webserver_forever(
    enum webserver_mode mode,
    struct server_settings* settings,
    struct request_handler* handlers,
    on_connection_created_cb on_connection_created,
    on_connection_destruction_cb on_connection_destruction)
{
    struct server server;
    enum webserver_errors error;

    sigaction(SIGPIPE, &(struct sigaction) { .sa_handler = SIG_IGN }, NULL);

    server_init(&server, settings, handlers, on_connection_created, on_connection_destruction);

    fprintf(stderr, "server listening on port:%d\n", server.port);

    error = start_listening(&server);
    if (error)
        return error;

    switch (mode) {
        case fork_mode:
            run_server_forked(&server);
            break;

        case thread_mode:
            run_server_threaded(&server);
            break;
        
        case thread_1_mode:
            run_server_1_thread(&server);
            break;

    }

    return no_fatal_errors;
}
