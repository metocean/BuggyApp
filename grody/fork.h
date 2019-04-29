#ifndef GRODY_WEBSERVER_FORK_H
#define GRODY_WEBSERVER_FORK_H

#ifdef __cplusplus
extern "C" {
#endif

#include "webserver.h"

void
run_server_forked(struct server *server);

#ifdef __cplusplus
}
#endif

#endif
