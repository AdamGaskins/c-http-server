#include "socket-server.h"

#include <assert.h>
#include <netinet/in.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/poll.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#define RETURN_ERR_IF_FAILED(a, server, msg) \
    if (a == -1) {                           \
        perror(msg);                         \
        SS_free(server);                     \
        return 0;                            \
    }                                        \
    static_assert(true, "require semi-colon")

#define RETURN_IF_FAILED(a, msg) \
    if (a == -1) {               \
        perror(msg);             \
        return;                  \
    }                            \
    static_assert(true, "require semi-colon")

struct socket_server* SS_open(uint16_t port, void (*message_received)(struct socket_server* server, struct socket_client*, char const*))
{
    struct socket_server* server = malloc(sizeof(struct socket_server));
    server->message_received = message_received;
    server->port = port;

    server->fd = socket(PF_INET, SOCK_STREAM, 0);
    RETURN_ERR_IF_FAILED(server->fd, server, "Failed creating socket");

    // Allow socket reuse to prevent "Address already in use" error.
    int yes = 1;
    int setsockopt_result = setsockopt(server->fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));
    RETURN_ERR_IF_FAILED(setsockopt_result, server, "Failed to setsockopt SO_REUSEADDR");

    struct sockaddr_in address = {
        .sin_family = AF_INET,
        .sin_port = htons(port),
        .sin_addr = { 0 }
    };
    int bind_result = bind(server->fd, (struct sockaddr*)&address, sizeof(address));
    RETURN_ERR_IF_FAILED(bind_result, server, "Failed to bind");

    int listen_result = listen(server->fd, 10);
    RETURN_ERR_IF_FAILED(listen_result, server, "Failed to listen");

    return server;
}

inline static void _populate_poll_clients(struct pollfd client_poll[MAX_SIMULTANEOUS_CONNECTIONS + 1], struct socket_server* server)
{
    client_poll[0] = (struct pollfd) { .fd = server->fd, .events = POLLIN };

    for (int i = 0; i < server->client_count; i++) {
        client_poll[i + 1] = (struct pollfd) {
            .fd = server->clients[i].fd,
            .events = POLLIN | POLLHUP
        };
    }
}

static void _accept_new_client(struct socket_server* server)
{
    struct sockaddr clientaddr;
    socklen_t clientaddr_size;
    int clientfd = accept(server->fd, &clientaddr, &clientaddr_size);
    RETURN_IF_FAILED(clientfd, "Failed to accept client");

    if (server->client_count + 1 == MAX_SIMULTANEOUS_CONNECTIONS) {
        // TODO: Expose function for max connections reached.
        fprintf(stderr, "Max simultaneous connections reached.\n");
        close(clientfd);
        return;
    }

    printf("Client connected.\n");

    server->clients[server->client_count++] = (struct socket_client) {
        .fd = clientfd,
        .disconnected = false
    };
}

static void _receive_from_client(struct socket_server* server, struct socket_client* client, struct pollfd poll_client)
{
    assert(poll_client.fd == client->fd && "Clients misaligned somehow!");

    if (poll_client.revents & POLLHUP) {
        client->disconnected = true;
        return;
    }

    // TODO: Allow more than 4096 bytes to be received.
    char buf[4096] = { 0 };
    int byte_count = recv(client->fd, buf, 4096 - 1, 0);
    RETURN_IF_FAILED(byte_count, "Failed to recv from client");

    if (byte_count == 0) {
        client->disconnected = true;
        return;
    }

    server->message_received(server, client, buf);
}

static void _handle_poll(struct socket_server* server, struct pollfd poll_results[MAX_SIMULTANEOUS_CONNECTIONS + 1])
{
    for (int i = 0; i < server->client_count + 1; i++) {
        if (!(poll_results[i].revents & (POLLIN | POLLHUP))) {
            continue;
        }

        if (poll_results[i].fd == server->fd) {
            _accept_new_client(server);
        } else {
            _receive_from_client(server, &server->clients[i - 1], poll_results[i]);
        }
    }

    // disconnect clients
    for (int i = 1; i < server->client_count + 1; i++) {
        if (!server->clients[i - 1].disconnected) {
            continue;
        }

        server->clients[i - 1] = server->clients[--server->client_count];
        printf("Client disconnected.\n");
    }
}

void SS_handle(struct socket_server* server)
{
    struct pollfd client_poll[MAX_SIMULTANEOUS_CONNECTIONS + 1];

    for (;;) {
        _populate_poll_clients(client_poll, server);

        int count = poll(client_poll, server->client_count + 1, -1);

        // timed out. poll again.
        if (count == 0) {
            continue;
        }

        if (count == -1) {
            perror("Failed to poll");
            continue;
        }

        _handle_poll(server, client_poll);
    }
}

int SS_send(struct socket_client* client, char const* message)
{
    int message_len = strlen(message);
    int sent = 0;
    int remaining = message_len;

    while (sent < message_len) {
        int x = send(client->fd, message, remaining, 0);

        if (x == -1) {
            perror("Failed to send");
            client->disconnected = true;
            return -1;
        }

        sent += x;
        remaining -= message_len;
    }

    return 0;
}

void SS_free(struct socket_server* server)
{
    close(server->fd);
    free(server);
    server = 0;
}
