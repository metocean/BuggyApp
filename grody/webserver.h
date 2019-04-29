#ifndef GRODY_WEBSERVER_H
#define GRODY_WEBSERVER_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>
#include <unistd.h>
#include <netinet/in.h>
#include <stdbool.h>

#include "picohttpparser/picohttpparser.h"

extern const char* mime_text;
extern const char* mime_json;
extern const char* mime_binary;
extern const char* mime_png;
extern const char* mime_jpeg;

enum http_status {
    http_continue                  = 100,
    http_switching_protocols       = 101,
    http_processing                = 102,
    http_early_hint                = 103,

    http_ok                        = 200,
    http_created                   = 201,
    http_accepted                  = 202,
    http_no_content                = 204,
    http_reset_content             = 205,
    http_partial_content           = 206,

    http_multiple_choices          = 300,
    http_moved_permanently         = 301,
    http_found                     = 302,
    http_see_other                 = 303,
    http_not_modified              = 304,
    http_use_proxy                 = 305,
    http_switch_proxy              = 306,
    http_temporary_redirect        = 307,
    http_permanent_redirect        = 308,

    http_bad_request               = 400,
    http_unauthorized              = 401,
    http_payment_required          = 402,
    http_forbidden                 = 403,
    http_not_found                 = 404,
    http_not_allowed               = 405,
    http_not_acceptable            = 406,
    http_proxy_auth_required       = 407,
    http_request_timeout           = 408,
    http_conflict                  = 409,
    http_gone                      = 410,
    http_length_required           = 411,
    http_precondition_failed       = 412,
    http_request_entity_too_large  = 413,
    http_request_uri_too_large     = 414,
    http_unsupported_media_type    = 415,
    http_range_not_satisfiable     = 416,
    http_misdirected_request       = 421,
    http_too_many_requests         = 429,

    http_internal_server_error     = 500,
    http_not_implemented           = 501,
    http_bad_gateway               = 502,
    http_service_unavailable       = 503,
    http_gateway_timeout           = 504,
    http_version_not_supported     = 505,
    http_network_auth_required     = 511
};

extern const char* http_continue_text;
extern const char* http_switching_protocols_text;
extern const char* http_processing_text;
extern const char* http_early_hint_text;

extern const char* http_ok_text;
extern const char* http_created_text;
extern const char* http_accepted_text;
extern const char* http_unauthorized_text;
extern const char* http_no_content_text;
extern const char* http_reset_content_text;
extern const char* http_partial_content_text;

extern const char* http_multiple_choices_text;
extern const char* http_moved_permanently_text;
extern const char* http_found_text;
extern const char* http_see_other_text;
extern const char* http_not_modified_text;
extern const char* http_use_proxy_text;
extern const char* http_switch_proxy_text;
extern const char* http_temporary_redirect_text;
extern const char* http_permanent_redirect_text;

extern const char* http_internal_server_error_text;
extern const char* http_not_implemented_text;
extern const char* http_bad_gateway_text;
extern const char* http_service_unavailable_text;
extern const char* http_gateway_timeout_text;
extern const char* http_version_not_supported_text;
extern const char* http_network_auth_required_text;

struct server_settings {
    // server port to listen on.
    uint16_t port;
    // how many connections can be waiting in the TCP / kernel stack before
    // rejecting more connections.
    int accept_backlog;
    // how long before the server gives up wait for the client to send data. (seconds)
    int client_read_timeout;
    // how long before the server gives up trying to send data to the client. (seconds)
    int client_write_timeout;
    // how long the server will wait trying to send or read data have the socket
    // / client has been shutdown. (seconds)
    int client_socket_linger;

    // a buffer allocate for every request used for reading the http request header.
    // can also contain the request some or all of body as well,
    // if you make it bigger this is more likely.
    size_t client_buffer_size;
    // if the request contains a body (via POST or PUT verbs) this is the maximum
    // size it will read to before you need to read in the request handler.
    size_t client_body_max_size;
    // if the request contains a body (via POST or PUT verbs) this is the
    // incremental size it will increase the body buffer size by until it
    // reaches or exceeds client_body_max_size. After which you will need to
    // read the body in your request handler.
    size_t client_body_increment_size;
};

enum webserver_fatal_errors {
    no_fatal_errors = 0,
    listener_socket_error = 1,
    listener_socket_options_error = 2,
    listener_bind_error = 3,
    listener_listen_error = 4
};

enum webserver_errors {

    successful = 0,
    client_disconnected_error = 1,
    client_read_timeout = 2,

    request_reading_error = 3,
    request_parsing_error = 4,
    response_write_timeout = 5,
    response_writing_error = 6,

    force_disconnect_error = 7,

    request_to_big_error = 8,
    request_content_len_and_transfer_encoding_error = 9,
    request_transfer_decoding_error = 10,
    out_of_memory_error = 11,
    not_implemented = 12
};


enum webserver_mode {
    fork_mode = 0,
    thread_mode = 1,
    thread_1_mode = 2
};

struct request {

    char *_buffer;

    size_t _buffer_used;
    size_t _buffer_size;

    // using for mallocing.
    char *_body;

    // body can be either _buffer or _body.
    const char *body;
    size_t body_size;

    bool is_chunked_encoded;
    ssize_t content_length;

    struct phr_chunked_decoder chunk_decoder;
    bool has_more_body;

    int header_size;

    const char *method;
    size_t method_len;

    const char *url;
    size_t url_len;

    const char *param;
    size_t param_len;

    const char *connection;
    size_t connection_len;

    const char *content_type;
    size_t content_type_len;

    const char *transfer_encoding;
    size_t transfer_encoding_len;

    const char *keep_alive;
    size_t keep_alive_len;

    int minor_version;

    struct phr_header headers[128];
    size_t num_headers;
};

struct client {
    // Internal variables not to be used outside of lib.
    const struct server *_server;
    struct sockaddr _client_addr;
    int _keep_alive;

    // fd to the client's socket.
    int sock;

    // the request and it's headers
    struct request req;

    // last server error related to the client.
    enum webserver_errors error;

    // last client system / io error detected.
    int erron;

    // the server's user can set and desorty this in the callbacks
    // on_connection_created() and on_connection_destruction()
    void *data;
    size_t data_size;

};

typedef void (*request_handler_cb)(struct client *client);

struct request_handler {
    // method verb.
    const char *method;
    // null terminated string of the URI path the handler while handle.
    // use for default "*".
    const char *uri;

    request_handler_cb handler;
};

typedef void (*on_connection_created_cb)(struct client *client);
typedef void (*on_connection_destruction_cb)(struct client *client);

struct server {
    uint16_t port;
    int accept_backlog;

    struct timeval client_read_timeout;
    struct timeval client_write_timeout;
    struct linger client_linger;

    int send_buffer_size;

    size_t client_buffer_size;
    size_t client_body_max_size;
    size_t client_body_increment_size;

    struct sockaddr_in addr;
    int addr_len;
    int sock;

    struct request_handler* request_handlers;
    on_connection_created_cb on_connection_created;
    on_connection_destruction_cb on_connection_destruction;
};

enum webserver_fatal_errors
run_webserver_forever(
    enum webserver_mode mode,
    struct server_settings* settings,
    struct request_handler* handlers,
    on_connection_created_cb on_connection_created,
    on_connection_destruction_cb on_connection_destruction
);

// returns 0 if successful, -1 if it fails for some reason.
int
not_found_response(struct client *client);

// sends the http response header, using this implies you are likely to use
// client_send() after using start_response().
int
start_response(struct client *client, int status, const char* status_text,
    const char *content_type, size_t content_length);

// send a full response including the buffer.
int
respond(struct client *client, int status, const char* status_text,
        const char *content_type, const void *content, size_t content_length);

// send a full response using varargs and a formatter.
int
respondf(struct client *client,
        int status,
        const char* status_text,
        const char *content_type,
        const char *format, ...);


// send buffer to the client socket.
// returns 0 if successful, -1 if it fails for some reason.
int
client_send(struct client* client, const void* buffer, size_t buffer_size);


// Does a single recv on the client socket.
// returns 0 if successful, -1 if it fails for some reason.
int
client_read(struct client* client, char *buffer, size_t size, ssize_t *read);


// read socket until x-bytes (i.e. size param).
// returns 0 if successful, -1 if it fails for some reason.
int
client_read_until(struct client* client, char *buffer, ssize_t size);


// gracefully disconnect the client from the server after returning from the
// request handler.
void
client_shutdown(struct client* client);


// not so gracefully disconnect the client from the server after returning from
// the request handler.
void
client_disconnect(struct client* client);


void
server_settings_defaults(struct server_settings* settings);


void
client_loop(struct client* client);

struct client *
client_create(const struct server *server, int sock);

void
client_free(void *arg);

#ifdef TEST

void
server_init(
    struct server* server,
    struct server_settings* settings,
    struct request_handler* handlers,
    on_connection_created_cb on_connection_created,
    on_connection_destruction_cb on_connection_destruction);





#endif

#ifdef __cplusplus
}
#endif

#endif
