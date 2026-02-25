#include "filemanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <rpc/pmap_clnt.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int _rpcsvcdirty = 0;

extern void filemanager_prog_1(struct svc_req *rqstp, SVCXPRT *transp);

SVCXPRT *filemanager_server_start(int port) {
    SVCXPRT *transp;
    int sock;
    struct sockaddr_in addr;
    int opt = 1;

    if (port > 0) {
        sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            fprintf(stderr, "ERROR: Cannot create socket\n");
            return NULL;
        }

        setsockopt(sock, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        memset(&addr, 0, sizeof(addr));
        addr.sin_family = AF_INET;
        addr.sin_addr.s_addr = INADDR_ANY;
        addr.sin_port = htons(port);

        if (bind(sock, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
            fprintf(stderr, "ERROR: Cannot bind to port %d\n", port);
            close(sock);
            return NULL;
        }

        transp = svctcp_create(sock, 0, 0);
        if (transp == NULL) {
            fprintf(stderr, "ERROR: Cannot create TCP service\n");
            close(sock);
            return NULL;
        }

        if (!svc_register(transp, FILEMANAGER_PROG, FILEMANAGER_VERS, filemanager_prog_1, 0)) {
            fprintf(stderr, "ERROR: Cannot register service\n");
            return NULL;
        }

        printf("File Manager Server started on port %d\n", port);
        printf("Connect with: ./bin/filemanager_client localhost:%d\n", port);
    } else {
        pmap_unset(FILEMANAGER_PROG, FILEMANAGER_VERS);

        transp = svctcp_create(RPC_ANYSOCK, 0, 0);
        if (transp == NULL) {
            fprintf(stderr, "ERROR: Cannot create TCP service\n");
            return NULL;
        }

        if (!svc_register(transp, FILEMANAGER_PROG, FILEMANAGER_VERS, filemanager_prog_1, IPPROTO_TCP)) {
            fprintf(stderr, "ERROR: Cannot register service (portmapper needed)\n");
            return NULL;
        }

        printf("File Manager Server started (using portmapper)\n");
        printf("Use 'rpcinfo -p localhost' to find the port\n");
    }

    return transp;
}
