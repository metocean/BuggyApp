// main.cpp
#include <string>
#include <iostream>
#include <string>
#include "grody/webserver.h"

void consumeMemory(struct client *);
void exitEmulation(struct client *);
void hangEmulation(struct client *);
void on_connection_created(struct client *);
void on_connection_destruction(struct client *);

int
main(int /* argc */, char ** /* argv[] */)
{
    struct request_handler handlers[] = {{"GET", "/memory", consumeMemory},
        {"GET", "/exit", exitEmulation}, {"GET", "/hang", hangEmulation}, {}};

    struct server_settings settings = {};
    server_settings_defaults(&settings);
    settings.port = 8080;

    std::cout << "Started..." << std::endl;

    if(run_webserver_forever(thread_1_mode /*fork_mode*/, &settings, handlers, on_connection_created,
           on_connection_destruction)) {
        return 1;
    }
}

void
consumeMemory(struct client * client)
{
    std::cout << "consumeMemory" << std::endl;
    unsigned int sz = rand() * 1024;
    new char[sz]; // 10Mb
    const std::string r = std::to_string(sz / 1024.0 / 1024) + "Mb memory consumed!\n";
    client_send(client, r.c_str(), r.size());
}

void
exitEmulation(struct client *)
{
    std::cout << "exitEmulation" << std::endl;
    exit(0);
}

void
hangEmulation(struct client *)
{
    std::cout << "hangEmulation" << std::endl;
    while(1) {
        /* forever */
    }
}

void
on_connection_created(struct client *)
{
    std::cout << "on_connection_created" << std::endl;
    // nothing todo
}

void
on_connection_destruction(struct client *)
{
    std::cout << "on_connection_destruction" << std::endl;
    // nothing todo
}
