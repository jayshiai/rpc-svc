#include "filemanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <signal.h>

#define STORAGE_DIR "storage"

extern void filemanager_prog_1(struct svc_req *rqstp, SVCXPRT *transp);
extern SVCXPRT *filemanager_server_start(int port);

static char storage_path[1024];
static int storage_initialized = 0;

static void ensure_storage_dir(void) {
    if (!storage_initialized) {
        getcwd(storage_path, sizeof(storage_path));
        strcat(storage_path, "/");
        strcat(storage_path, STORAGE_DIR);
        mkdir(storage_path, 0755);
        storage_initialized = 1;
    }
}

static void get_full_path(const char *filename, char *fullpath) {
    ensure_storage_dir();
    snprintf(fullpath, 256, "%s/%s", storage_path, filename);
}

static int sanitize_filename(const char *input) {
    if (!input || strlen(input) == 0 || strlen(input) >= 256) return -1;
    if (strstr(input, "..") != NULL) return -1;
    return 0;
}

int *upload_file_1_svc(UploadArgs *argp, struct svc_req *rqstp) {
    static int result;
    char fullpath[256];
    FILE *fp;

    result = -1;
    if (sanitize_filename(argp->filename) != 0) return &result;

    get_full_path(argp->filename, fullpath);
    fp = fopen(fullpath, "wb");
    if (!fp) return &result;

    fwrite(argp->data.data_val, 1, argp->data.data_len, fp);
    fclose(fp);
    result = 0;
    return &result;
}

DownloadResult *download_file_1_svc(char **argp, struct svc_req *rqstp) {
    static DownloadResult result;
    char fullpath[256];
    FILE *fp;
    long filesize;

    memset(&result, 0, sizeof(result));
    result.success = 0;

    if (sanitize_filename(*argp) != 0) return &result;

    get_full_path(*argp, fullpath);
    fp = fopen(fullpath, "rb");
    if (!fp) return &result;

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (filesize > 10485760) { fclose(fp); return &result; }

    result.data.data_val = malloc(filesize);
    result.data.data_len = fread(result.data.data_val, 1, filesize, fp);
    fclose(fp);
    result.success = 1;
    return &result;
}

FileList *list_files_1_svc(void *argp, struct svc_req *rqstp) {
    static FileList result;
    static char files_buffer[25600];
    DIR *dir;
    struct dirent *entry;
    int offset = 0;

    ensure_storage_dir();
    memset(&result, 0, sizeof(result));
    memset(files_buffer, 0, sizeof(files_buffer));

    dir = opendir(storage_path);
    if (!dir) return &result;

    result.count = 0;
    result.files = files_buffer;

    while ((entry = readdir(dir)) != NULL && result.count < 100) {
        if (entry->d_type == DT_REG) {
            int len = strlen(entry->d_name);
            if (offset + len + 2 < 25600) {
                if (offset > 0) files_buffer[offset++] = '\n';
                strcpy(files_buffer + offset, entry->d_name);
                offset += len;
                result.count++;
            }
        }
    }
    closedir(dir);
    return &result;
}

int *delete_file_1_svc(char **argp, struct svc_req *rqstp) {
    static int result;
    char fullpath[256];

    result = -1;
    if (sanitize_filename(*argp) != 0) return &result;

    get_full_path(*argp, fullpath);
    result = remove(fullpath);
    return &result;
}

static void sig_handler(int sig) {
    exit(0);
}

int main(int argc, char **argv) {
    SVCXPRT *transp;
    int port = 0;

    if (argc > 1) {
        port = atoi(argv[1]);
    }

    signal(SIGINT, sig_handler);
    signal(SIGTERM, sig_handler);

    transp = filemanager_server_start(port);
    if (transp == NULL) exit(1);

    printf("Press Ctrl+C to stop...\n");
    svc_run();
    fprintf(stderr, "ERROR: svc_run returned\n");
    return 1;
}
