#include "filemanager.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <rpc/rpc.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

static struct timeval TIMEOUT = { 25, 0 };

CLIENT *filemanager_connect(const char *hostspec) {
    char host[256];
    int port = 0;
    char *colon;
    struct sockaddr_in server_addr;
    struct hostent *he;
    int sock;
    CLIENT *clnt;

    strncpy(host, hostspec, sizeof(host) - 1);
    host[sizeof(host) - 1] = '\0';

    colon = strchr(host, ':');
    if (colon) {
        *colon = '\0';
        port = atoi(colon + 1);
    }

    if (port > 0) {
        he = gethostbyname(host);
        if (he == NULL) {
            fprintf(stderr, "ERROR: Cannot resolve hostname %s\n", host);
            return NULL;
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        memcpy(&server_addr.sin_addr, he->h_addr, he->h_length);
        server_addr.sin_port = htons(port);

        sock = RPC_ANYSOCK;
        clnt = clnttcp_create(&server_addr, FILEMANAGER_PROG, FILEMANAGER_VERS, &sock, 0, 0);
        if (clnt == NULL) {
            clnt_pcreateerror(host);
            return NULL;
        }
    } else {
        clnt = clnt_create(host, FILEMANAGER_PROG, FILEMANAGER_VERS, "tcp");
        if (clnt == NULL) {
            clnt_pcreateerror(host);
            return NULL;
        }
    }

    return clnt;
}

int *upload_file_1(UploadArgs *argp, CLIENT *clnt) {
    static int clnt_res;
    memset(&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call(clnt, UPLOAD_FILE, (xdrproc_t)xdr_UploadArgs, (caddr_t)argp,
                  (xdrproc_t)xdr_int, (caddr_t)&clnt_res, TIMEOUT) != RPC_SUCCESS)
        return NULL;
    return &clnt_res;
}

DownloadResult *download_file_1(char **argp, CLIENT *clnt) {
    static DownloadResult clnt_res;
    memset(&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call(clnt, DOWNLOAD_FILE, (xdrproc_t)xdr_wrapstring, (caddr_t)argp,
                  (xdrproc_t)xdr_DownloadResult, (caddr_t)&clnt_res, TIMEOUT) != RPC_SUCCESS)
        return NULL;
    return &clnt_res;
}

FileList *list_files_1(void *argp, CLIENT *clnt) {
    static FileList clnt_res;
    memset(&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call(clnt, LIST_FILES, (xdrproc_t)xdr_void, (caddr_t)argp,
                  (xdrproc_t)xdr_FileList, (caddr_t)&clnt_res, TIMEOUT) != RPC_SUCCESS)
        return NULL;
    return &clnt_res;
}

int *delete_file_1(char **argp, CLIENT *clnt) {
    static int clnt_res;
    memset(&clnt_res, 0, sizeof(clnt_res));
    if (clnt_call(clnt, DELETE_FILE, (xdrproc_t)xdr_wrapstring, (caddr_t)argp,
                  (xdrproc_t)xdr_int, (caddr_t)&clnt_res, TIMEOUT) != RPC_SUCCESS)
        return NULL;
    return &clnt_res;
}
