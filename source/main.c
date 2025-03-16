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
    uint16_t header_count;
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

int parse_http_header(struct http_header* header, char const* data, uint16_t* i)
{
    if (data[*i] == 0 || data[*i] == '\r') {
        return 0;
    }

    int start = *i;
    int end = *i;
    int datalen = strlen(data);
    int colonIndex = 0;

    for (; *i < datalen; (*i)++) {
        if (data[*i] == ':' && colonIndex == 0) {
            colonIndex = *i;
            continue;
        }

        if (data[*i] == 0 || data[*i] == '\r') {
            end = *i;

            if (data[*i] == '\r') {
                (*i) += 2;
            }

            break;
        }
    }

    if (colonIndex == 0) {
        return 0;
    }

    int fieldlen = colonIndex - start;
    header->field = calloc(fieldlen + 1, sizeof(char));
    strncpy(header->field, &data[start], fieldlen);

    int valuelen = end - colonIndex - 2; // don't include ": "
    header->value = calloc(valuelen + 1, sizeof(char));
    strncpy(header->value, &data[colonIndex + 2], valuelen);

    return 1;
}

struct http_request* parse_http_request(char const* data)
{
    struct http_request* req = malloc(sizeof(*req));

    uint16_t i = 0;

    // parse
    req->method = string_to_method(parse_string(data, &i));
    req->url = parse_string(data, &i);
    req->protocol = parse_string(data, &i);

    req->headers = calloc(256, sizeof(*req->headers));
    for (req->header_count = 0; req->header_count < 256; req->header_count++) {
        int success = parse_http_header(&req->headers[req->header_count], data, &i);

        if (success == 0) {
            break;
        }
    }

    return req;
}

void message_received(struct socket_client* client, char const* message)
{
    struct http_request* req = parse_http_request(message);
    printf("\n> %s %s %s\n", method_to_string(req->method), req->url, req->protocol);
    for (int i = 0; i < req->header_count; i++) {
        printf("> %s: %s\n", req->headers[i].field, req->headers[i].value);
    }
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
