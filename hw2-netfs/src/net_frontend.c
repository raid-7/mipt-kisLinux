#include <rfsnet/net.h>
#include <myutil.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>


char* split_address(const char* addr, int* port) {
    char* pos = strchr(addr, ':');
    if (!pos) {
        die_fatal("Invalid address");
    }
    if (!sscanf(pos + 1, "%d", port)) {
        die_fatal("Invalid address");
    }

    char* host = malloc(pos - addr + 1);
    die("Memory allocation failed");

    memcpy(host, addr, pos - addr);
    *(host + (pos - addr)) = 0;

    return host;
}

int initialize_client(const char* address) {
    int port;
    char* host = split_address(address, &port);

    int sockd = socket(AF_INET, SOCK_STREAM, 0);
    die("Socket error");

    struct hostent* hostent = gethostbyname(host);
    in_addr_t* ip = (in_addr_t*) (*hostent->h_addr_list);

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(struct sockaddr_in));
    addr.sin_addr.s_addr = *ip;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);

    connect(sockd, (const struct sockaddr*) &addr, sizeof(struct sockaddr_in));
    die("Socket error");

    free(host);

    return sockd;
}

void destroy_client(int sockd) {
    close(sockd);
}
