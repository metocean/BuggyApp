// main.cpp
#include <string>
#include <iostream>
#include <string>
#include "grody/webserver.h"

void defaultRespond(client *){}
void getId(client *);
void consumeMemory(client *);
void healthCheck(client *);
void exitEmulation(client *);
void hangEmulation(client *);
void on_connection_created(client *);
void on_connection_destruction(client *);

int
main(int /* argc */, char ** /* argv[] */)
{
    struct request_handler handlers[] = {{"GET", "/id", getId}, {"GET", "/health", healthCheck},
        {"GET", "/memory", consumeMemory}, {"GET", "/exit", exitEmulation},
        {"GET", "/hang", hangEmulation}, {"GET", "", defaultRespond}, {}};

    struct server_settings settings = {};
    server_settings_defaults(&settings);
    settings.port = 8080;

    std::cout << "Started..." << std::endl;

    if(run_webserver_forever(single_thread_mode, &settings, handlers, on_connection_created,
           on_connection_destruction)) {
        return 1;
    }
}

void
sendstr(client * client, const std::string & reply)
{
    std::cout << reply << std::endl;
    respond(client, http_ok, http_ok_text, mime_text, reply.c_str(), reply.size());
    client_shutdown(client);
}

void
healthCheck(client * client)
{
    sendstr(client, "Ok\n");
}

void
getId(client * client)
{
    sendstr(client, "getId:" + std::to_string(pthread_self()) + "\n");
}

#include <vector>
void
consumeMemory(client * client)
{
    auto v
        = new std::vector<char>(rand() % 200000000, 1); // It throws, so it'll crash - good here :)
    auto ssz = std::to_string(v->size());

    const int t = ssz.size();
    if(t > 3)
        ssz.insert(t - 3, "K ");
    if(t > 6)
        ssz.insert(t - 6, "M ");
    if(t > 9)
        ssz.insert(t - 9, "G ");

    sendstr(client, "consumeMemory:" + ssz + " bytes\n");
}

void
exitEmulation(client * client)
{
    sendstr(client, "exitEmulation\n");

    int no_delay = 1;
    setsockopt(client->sock, IPPROTO_TCP, 1, (char *)&no_delay, sizeof(int));
    shutdown(client->sock, SHUT_RDWR);
    exit(-1);
}

void
hangEmulation(client * client)
{
    std::cout << "Hang" << std::endl;
    client_send(client, "Hang", 4);
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
