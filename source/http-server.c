#include "http-server.h"
#include "socket-server.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

char* HTTP_method_to_string(enum http_method method)
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

enum http_method HTTP_method_from_string(char const* method)
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

char* HTTP_status_to_string(enum http_status status)
{
    switch (status) {
    case RESPONSE_200_OK:
        return "200 OK";
    case RESPONSE_404_NOT_FOUND:
        return "404 NOTFOUND";
    default:
        return "";
    }
}

static char const* _parse_string(char const* data, uint16_t* i)
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

int _parse_http_header(struct http_header* header, char const* data, uint16_t* i)
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

struct http_request* _parse_http_request(char const* data)
{
    struct http_request* req = malloc(sizeof(*req));

    uint16_t i = 0;

    // parse
    req->method = HTTP_method_from_string(_parse_string(data, &i));
    req->url = _parse_string(data, &i);
    req->protocol = _parse_string(data, &i);

    req->headers = calloc(256, sizeof(*req->headers));
    for (req->header_count = 0; req->header_count < 256; req->header_count++) {
        int success = _parse_http_header(&req->headers[req->header_count], data, &i);

        if (success == 0) {
            break;
        }
    }

    return req;
}

void _free_http_request(struct http_request* req)
{
    free(req);
}

char* _readfile(char* path)
{
    char* buffer = 0;
    long length;
    FILE* f = fopen(path, "rb");

    if (f) {
        fseek(f, 0, SEEK_END);
        length = ftell(f);
        fseek(f, 0, SEEK_SET);
        buffer = malloc(length);
        if (buffer) {
            fread(buffer, 1, length, f);
        }
        fclose(f);
    }

    return buffer;
}

void _socket_message_received(struct socket_server* socket_server, struct socket_client* client, char const* message)
{
    struct http_server* server = (struct http_server*)socket_server->data;

    struct http_request* req = _parse_http_request(message);
    req->client = client;

    printf("\n> %s %s %s\n", HTTP_method_to_string(req->method), req->url, req->protocol);
    for (int i = 0; i < req->header_count; i++) {
        printf("> %s: %s\n", req->headers[i].field, req->headers[i].value);
    }

    bool response_sent = false;
    for (int i = 0; i < server->route_count; i++) {
        struct http_route* route = &server->routes[i];

        if (route->method != req->method)
            continue;

        if (strcmp(route->url, req->url) == 0) {
            char* contents = _readfile(route->directory);

            struct http_response response = {
                .status = RESPONSE_200_OK,
                .body = contents
            };

            HTTP_send_response(req, response);
            response_sent = true;
            break;
        }
    }

    if (!response_sent) {
        HTTP_send_response(req, (struct http_response) { .status = RESPONSE_404_NOT_FOUND, .body = "Not found." });
    }

    _free_http_request(req);
}

struct http_server* HTTP_open(uint16_t port)
{
    struct http_server* server = calloc(1, sizeof(*server));

    struct socket_server* socket = SS_open(port, _socket_message_received);
    socket->data = server;

    if (socket == 0) {
        fprintf(stderr, "Failed to start server.\n");
        free(server);
        return 0;
    }

    server->socket = socket;
    server->routes = calloc(256, sizeof(*server->routes));

    return server;
}

void HTTP_handle(struct http_server* server)
{
    printf("Listening at http://localhost:%d\n", server->socket->port);

    SS_handle(server->socket);
}

void HTTP_register_directory(struct http_server* server, char* url, char* directory)
{
    struct http_route* route = calloc(1, sizeof(*route));

    route->method = METHOD_GET;
    route->type = ROUTE_DIRECTORY;
    route->url = url;
    route->directory = directory;

    server->routes[server->route_count++] = *route;
}

void HTTP_send_response(struct http_request* request, struct http_response response)
{
    size_t body_len = strlen(response.body);

    SS_send(request->client, "HTTP/1.1 ");
    SS_send(request->client, HTTP_status_to_string(response.status));
    SS_send(request->client, "\nContent-Type: text/html");
    SS_send(request->client, "\nContent-Length: ");

    char str[100] = { 0 };
    sprintf(str, "%zd", body_len);
    SS_send(request->client, str);

    SS_send(request->client, "\n\n");
    SS_send(request->client, response.body);
}

void HTTP_free(struct http_server* server)
{
    SS_free(server->socket);
    free(server);
    server = 0;
}
