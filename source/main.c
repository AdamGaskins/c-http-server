#include "socket-server.h"
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

enum request_method {
    METHOD_NONE,
    METHOD_GET,
    METHOD_HEAD,
    METHOD_POST,
    METHOD_PUT,
    METHOD_DELETE,
    METHOD_CONNECT,
    METHOD_OPTIONS,
    METHOD_TRACE,
    METHOD_PATCH
};

char* method_to_string(enum request_method method)
{
    switch (method) {
    case METHOD_GET:
        return "GET";
    case METHOD_HEAD:
        return "HEAD";
    case METHOD_POST:
        return "POST";
    case METHOD_PUT:
        return "PUT";
    case METHOD_DELETE:
        return "DELETE";
    case METHOD_CONNECT:
        return "CONNECT";
    case METHOD_OPTIONS:
        return "OPTIONS";
    case METHOD_TRACE:
        return "TRACE";
    case METHOD_PATCH:
        return "PATCH";
    default:
        return "";
    }
}

enum request_method string_to_method(char const* method)
{
    if (strcmp(method, "GET") == 0)
        return METHOD_GET;
    if (strcmp(method, "HEAD") == 0)
        return METHOD_HEAD;
    if (strcmp(method, "POST") == 0)
        return METHOD_POST;
    if (strcmp(method, "PUT") == 0)
        return METHOD_PUT;
    if (strcmp(method, "DELETE") == 0)
        return METHOD_DELETE;
    if (strcmp(method, "CONNECT") == 0)
        return METHOD_CONNECT;
    if (strcmp(method, "OPTIONS") == 0)
        return METHOD_OPTIONS;
    if (strcmp(method, "TRACE") == 0)
        return METHOD_TRACE;
    if (strcmp(method, "PATCH") == 0)
        return METHOD_PATCH;
    return METHOD_NONE;
}

struct http_header {
    char* field;
    char* value;
};

struct http_request {
    enum request_method method;
    char const* url;
    char const* protocol;
    struct http_header* headers;
};

static char const* parse_string(char const* data, uint16_t* i)
{
    int l = 0;
    // loop until we hit a boundary character
    for (;; l++) {
        if (data[*i + l] == 0 || data[*i + l] == ' ' || data[*i + l] == '\n')
            break;
    }

    char* string = calloc(1, sizeof(char) * (l + 1));

    strncpy(string, &data[*i], l);

    // loop past the boundary characters
    for (;; l++) {
        if (data[*i + l] == 0)
            break;

        if (data[*i + l] == ' ' || data[*i + l] == '\n')
            continue;

        break;
    }
    (*i) += l;
    return string;
}

struct http_request* parse_http_request(char const* data)
{
    struct http_request* req = malloc(sizeof(*req));

    uint16_t i = 0;

    // parse
    req->method = string_to_method(parse_string(data, &i));
    req->url = parse_string(data, &i);
    req->protocol = parse_string(data, &i);

    return req;
}

void message_received(struct socket_client* client, char const* message)
{
    struct http_request* req = parse_http_request(message);
    printf("%s %s %s\n", method_to_string(req->method), req->url, req->protocol);
    free(req);

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
