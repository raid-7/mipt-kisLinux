#include <rfsnet/net.h>
#include <rfscore/manage.h>
#include <operation_defines.h>

#include <sys/socket.h>
#include <stdatomic.h>
#include <myutil.h>
#include <stdlib.h>
#include <unistd.h>
#include <syslog.h>
#include <errno.h>
#include <stdbool.h>
#include <pthread.h>


int current_sockd;
char error_intercepted;
void fsop_error(const char* s) {
    error_intercepted = 1;
    String str = to_string(s);
    if (slz_String_write(current_sockd, &str)) {
        shutdown(current_sockd, SHUT_RDWR);
    }
    free_string(str);
}


NetServerDescriptors initialize_server(int port) {
    NetServerDescriptors nds;
    nds.sockd = socket(AF_INET, SOCK_STREAM, 0);
    die("Unable to create socket");

    nds.addr.sin_family = AF_INET;
    nds.addr.sin_port = htons(port);
    nds.addr.sin_addr.s_addr = htonl(INADDR_ANY);

    bind(nds.sockd, (struct sockaddr*) &nds.addr, sizeof(nds.addr));
    die("Unable to bind socket");

    nds.termination_flag = malloc(sizeof(atomic_bool));
    die("Memory allocation error");
    atomic_init(nds.termination_flag, 0);

    nds.mutex = malloc(sizeof(pthread_mutex_t));
    die("Memory allocation error");
    pthread_mutex_init(nds.mutex, NULL);

    return nds;
}

void server_destroy(NetServerDescriptors nds) {
    close(nds.sockd);
    free(nds.termination_flag);
    pthread_mutex_destroy(nds.mutex);
    free(nds.mutex);
}


struct ServeClientRequest {
    int sockd;
    FsDescriptors fds;
    pthread_mutex_t* mutex;
};

void process_operation(int sockd, FsDescriptors fds, NetFsOperation* op) {
    _FSOP_calls(op, fds, sockd);
}

bool serve_next_operation(struct ServeClientRequest* req) {
    NetFsOperation op;
    if (!slz_NetFsOperation_read(req->sockd, &op)) {
        return 0;
    }

    pthread_mutex_lock(req->mutex);
    current_sockd = req->sockd;
    error_intercepted = 0;
    intercept_errors(fsop_error);
    process_operation(req->sockd, req->fds, &op);
    intercept_errors(NULL);
    pthread_mutex_unlock(req->mutex);

    _FSOP_clear(&op);
    return 1;
}

void* serve_client(void* args) {
    struct ServeClientRequest* req = args;

    while (serve_next_operation(req));

    close(req->sockd);
    free(args);
    return NULL;
}

void server_listen_connections(NetServerDescriptors nds, FsDescriptors fds) {
    listen(nds.sockd, 5);
    die("Socket error");

    while (!atomic_load(nds.termination_flag)) {
        int fd = accept(nds.sockd, NULL, 0);
        if (warn("Problem accepting client"))
            continue;

        struct ServeClientRequest* req = malloc(sizeof(struct ServeClientRequest));
        die("Memory allocation error");
        req->sockd = fd;
        req->fds = fds;
        req->mutex = nds.mutex;

        pthread_t thr;
        pthread_create(&thr, NULL, serve_client, req);
        pthread_detach(thr);
    }
}
