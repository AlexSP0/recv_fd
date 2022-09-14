#include <errno.h>
#include <inttypes.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>

#include "const.h"

int main(int argc, char **argv) {
    int ret = 0;
    int sock;
    struct sockaddr_un local;
    ssize_t bytes_wrote, bytes_read;
    size_t bytes_left;
    struct AddArgs args;
    int32_t result;
    char *argsptr = (char *)&args;

    memset(&args, 0, sizeof(args));
    args.x = atoi(argc > 1 ? argv[1] : "22");
    args.y = atoi(argc > 2 ? argv[2] : "20");

    sock = socket(AF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("add_client: socket");
        exit(2);
    }

    memset(&local, 0, sizeof(local));
    local.sun_family = AF_UNIX;
    strncpy(local.sun_path, ADD_SOCKET_NAME, sizeof(local.sun_path) - 1);

    ret = connect(sock, (const struct sockaddr *)&local, sizeof(local));
    if (ret) {
        perror("connect");
        exit(3);
    }

    bytes_left = sizeof(args);

    while (bytes_left > 0) {
        bytes_wrote = write(sock, argsptr, bytes_left);
        if (bytes_wrote < 0) {
            ret = errno;
            perror("client: write");
            goto out;
        } else if (0 == bytes_wrote) {
            fprintf(stderr, "premature EOF by server [1]\n");
            ret = 5;
            goto out;
        } else {
            bytes_left -= (size_t)bytes_wrote;
            argsptr += bytes_wrote;
        }
    }

    bytes_left = sizeof(result);
    argsptr = (char *)&result;
    while (bytes_left > 0) {
        bytes_read = read(sock, argsptr, bytes_left);
        if (bytes_read < 0) {
            ret = errno;
            perror("client: read");
            goto out;
        } else if (0 == bytes_read) {
            fprintf(stderr, "premature EOF by server [2]\n");
            ret = 7;
            goto out;
        } else {
            bytes_left -= (size_t)bytes_read;
            argsptr += bytes_read;
        }
    }
    printf("client: result: %" PRId32 "\n", result);
out:
    close(sock);
    return ret;
}

