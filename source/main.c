#include "http-server.h"
#include <stdio.h>

int main()
{
    struct http_server* serv = HTTP_open(8080);

    if (serv == 0) {
        fprintf(stderr, "Failed to start HTTP server.\n");
        return 1;
    }

    struct http_header headers[] = {
        /* (struct http_header) { .field = "Content-Type", .value = "text/html" } */
    };
    HTTP_register_file(serv, "/index.html", "html/index.html", headers, 0);

    HTTP_handle(serv);

    HTTP_free(serv);

    return 0;
}
