#include "socket-server.h"
#include <stdio.h>

void message_received(struct socket_client* client, char const* message)
{
    char data[] = "<html><body><h1>Hello World!</h1></body></html>";
    char header[] = "HTTP/1.1 200 OK\nContent-Length: 47";
    char response[1024];
    snprintf(response, sizeof(response), "%s\n\n%s", header, data);

    SS_send(client, response);
}

int main()
{
    struct socket_server* serv = SS_open(8080, message_received);

    if (serv == 0) {
        fprintf(stderr, "Failed to start server.");
        return -1;
    }

    printf("Listening at http://localhost:%d\n", 8080);

    SS_handle(serv);

    SS_free(serv);
}
