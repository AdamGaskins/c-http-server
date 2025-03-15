#pragma once

#include <stdint.h>
#include <stdbool.h>

#define MAX_SIMULTANEOUS_CONNECTIONS 20

struct socket_client {
    int fd;
    bool disconnected;
};

struct socket_server {
    int fd;
    void (*message_received)(struct socket_client*, char const*);
    uint8_t client_count;
    struct socket_client clients[MAX_SIMULTANEOUS_CONNECTIONS];
};

struct socket_server* SS_open(uint16_t port, void (*message_received)(struct socket_client*, char const*));
void SS_handle(struct socket_server* server);
int SS_send(struct socket_client* client, char const* message);
void SS_free(struct socket_server* server);
