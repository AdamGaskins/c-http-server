#include "http-server.h"
#include <stdio.h>

int main()
{
    struct http_server* serv = HTTP_open(8080);

    if (serv == 0) {
        fprintf(stderr, "Failed to start HTTP server.\n");
        return 1;
    }

    HTTP_register_file(serv, "/index.html", "html/index.html");

    HTTP_handle(serv);

    HTTP_free(serv);

    return 0;
}
