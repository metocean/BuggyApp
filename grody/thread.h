#ifndef GRODY_WEBSERVER_THREAD_H
#define GRODY_WEBSERVER_THREAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include "webserver.h"

void
run_server_threaded(struct server *server);

void
run_server_1_thread(struct server *server);

#ifdef __cplusplus
}
#endif
#endif
