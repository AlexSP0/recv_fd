#include <errno.h>
#include <inttypes.h>
#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/un.h>
#include <unistd.h>

#include "const.h"

#define BACKLOG 100

int handle_client(int sock) {
    int err = 0;
    int32_t result;
    struct AddArgs args;
    ssize_t bytes_read, bytes_wrote;
    size_t bytes_left = sizeof(args);
    char *argsptr = (char *)&args;

    memset(argsptr, 0, sizeof(args));

    while (bytes_left > 0) {
        bytes_read = read(sock, argsptr, bytes_left);
        if (bytes_read < 0) {
            err = errno;
            perror("read");
            goto out;
        } else if (0 == bytes_read) {
            fprintf(stderr, "%s: premature EOF\n", __func__);
            err = 1;
        } else {
            bytes_left -= (size_t)bytes_read;
            argsptr += bytes_read;
        }
    }
    printf("%s: got arguments: x=%" PRId32 ", y=%" PRId32 "\n",
        __func__, args.x, args.y);

    result = args.x + args.y;
    bytes_left = sizeof(result);
    argsptr = (char *)&result;

    while (bytes_left > 0) {
        bytes_wrote = write(sock, argsptr, bytes_left);
        if (bytes_wrote <= 0) {
            err = errno;
            perror("write");
            goto out;
        }
        bytes_left -= (size_t)bytes_wrote;
        argsptr += bytes_wrote;
    }
    printf("%s: send reply: %" PRId32 "\n", __func__, result);
out:
    close(sock);
    return err;
}

int main(int argc, char **argv) {
    int ret = 0;
    int listen_sock;
    struct sockaddr_un local;

    listen_sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (listen_sock < 0) {
        perror("socket");
        exit(2);
    }

    ret = unlink(ADD_SOCKET_NAME);
    if (ret < 0) {
        if (ENOENT != errno) {
            perror("unlink: ");
            exit(7);
        }
    }

    memset(&local, 0, sizeof(local));
    local.sun_family = AF_UNIX;
    strncpy(local.sun_path, ADD_SOCKET_NAME, sizeof(local.sun_path) - 1);

    ret = bind(listen_sock, (const struct sockaddr *)&local, sizeof(local));
    if (ret) {
        perror("bind");
        exit(3);
    }
    ret = listen(listen_sock, BACKLOG);
    if (ret) {
        perror("listen");
        exit(5);
    }

    for (;;) {
        int sock = -1;

        sock = accept(listen_sock, NULL, NULL);
        if (sock < 0) {
            perror("accept");
            continue;
        }

        pid_t newPr = fork();
        if(newPr == 0) {
            printf("New process!\n");
            handle_client(sock);
        }

    }
    return 0;
}
