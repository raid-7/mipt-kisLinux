#include <rfscore/manage.h>
#include <rfsnet/net.h>
#include <myutil.h>

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <syslog.h>
#include <strings.h>
#include <signal.h>
#include <pthread.h>


const char* HELP_STRING = "Usage:\n./rfsdaemon run <port> <path_to_fs>\n  or\n./rfsdaemon init <path_to_fs> <size>";

void die_with_help() {
    printf("%s\n", HELP_STRING);
    exit(0);
}

void init(const char* filename, size_t size) {
    FsDescriptors fs = init_fs(filename, size);
    printf("Ready: %lu blocks of %lu bytes\n", fs.superblock->blocks_count, fs.superblock->block_size);
    close_fs(fs);
}

void start_fs_daemon(int port, const char* filename) {
    FsDescriptors descriptors = open_fs(filename);
    NetServerDescriptors net = initialize_server(port);

    printf("Initialization succeeded. Daemonization...");

    daemon(0, 0);
    die("Daemonization failed");

    struct sigaction act;
    memset(&act, 0, sizeof(struct sigaction));
    act.sa_handler = SIG_IGN;
    act.sa_flags = 0;
    sigaction(SIGABRT, NULL, &act);

    openlog("rfsdaemon", LOG_PID, LOG_DAEMON);
    configure_error_logging(0, 1);

    server_listen_connections(net, descriptors);

    close_fs(descriptors);
    server_destroy(net);
    closelog();
}


int main(int argc, const char* argv[]) {
    configure_error_logging(1, 0);

    if (argc != 4)
        die_with_help();

    if (!strcasecmp(argv[1], "init")) {
        unsigned long long size;
        if (!sscanf(argv[3], "%llu", &size)) {
            die_fatal("Invalid argument");
        }

        init(argv[2], size);
        return 0;
    }

    if (!strcasecmp(argv[1], "run")) {
        int port;
        if (!sscanf(argv[2], "%d", &port)) {
            die_fatal("Invalid argument");
        }

        start_fs_daemon(port, argv[3]);
        return 0;
    }

    die_with_help();
}
