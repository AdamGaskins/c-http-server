#pragma once

#include "socket-server.h"
#include <stdint.h>

enum http_method {
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

enum http_route_type {
    ROUTE_DIRECTORY,
    ROUTE_CONTROLLER
};

enum http_status {
    RESPONSE_200_OK = 200,
    RESPONSE_404_NOT_FOUND = 404
};

struct http_header {
    char* field;
    char* value;
};

struct http_mime_type_mapping {
    char* extension;
    char* mimetype;
};

struct http_request {
    struct socket_client* client;
    enum http_method method;
    char const* url;
    char const* protocol;
    uint16_t header_count;
    struct http_header* headers;
};

struct http_response {
    enum http_status status;
    char* body;
    uint16_t header_count;
    struct http_header* headers;
};

struct http_server {
    struct socket_server* socket;
    uint16_t route_count;
    struct http_route* routes;
    uint16_t mimetype_count;
    struct http_mime_type_mapping* mimetypes;
};

struct http_route {
    enum http_method method;
    enum http_route_type type;
    uint16_t header_count;
    struct http_header* headers;
    char* url;
    char* file;
    void (*controller)(struct http_request*);
};

char* HTTP_method_to_string(enum http_method method);
enum http_method HTTP_method_from_string(char const* method);

struct http_server* HTTP_open(uint16_t port);
void HTTP_register_mimetype(struct http_server* server, char* extension, char* mimetype);
void HTTP_handle(struct http_server* server);
void HTTP_register_file(struct http_server* server, char* url, char* file, struct http_header* headers, uint16_t header_count);
void HTTP_register_directory(struct http_server* server, char* url, char* path);
void HTTP_send_response(struct http_request* request, struct http_response response);
void HTTP_free_response(struct http_response response);
void HTTP_free(struct http_server* server);
