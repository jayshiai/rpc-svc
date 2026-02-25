#include "filemanager.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#define DOWNLOAD_DIR "downloads"

extern CLIENT *filemanager_connect(const char *hostspec);

CLIENT *clnt = NULL;

void ensure_download_dir(void) {
    char path[1024];
    getcwd(path, sizeof(path));
    strcat(path, "/");
    strcat(path, DOWNLOAD_DIR);
    mkdir(path, 0755);
}

void upload_file(void) {
    UploadArgs args;
    char local_path[256];
    char server_name[256];
    FILE *fp;
    long filesize;
    int *result;

    printf("Enter local file path: ");
    fflush(stdout);
    if (!fgets(local_path, sizeof(local_path), stdin)) return;
    local_path[strcspn(local_path, "\n")] = 0;

    fp = fopen(local_path, "rb");
    if (!fp) { printf("Error: File not found\n"); return; }

    printf("Enter server filename: ");
    fflush(stdout);
    if (!fgets(server_name, sizeof(server_name), stdin)) { fclose(fp); return; }
    server_name[strcspn(server_name, "\n")] = 0;
    if (strlen(server_name) == 0) strcpy(server_name, local_path);

    fseek(fp, 0, SEEK_END);
    filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);

    if (filesize > 10485760) { printf("Error: File too large\n"); fclose(fp); return; }

    args.filename = server_name;
    args.data.data_val = malloc(filesize);
    args.data.data_len = fread(args.data.data_val, 1, filesize, fp);
    fclose(fp);

    result = upload_file_1(&args, clnt);
    if (result == NULL) clnt_perror(clnt, "RPC failed");
    else if (*result == 0) printf("Uploaded successfully (%u bytes)\n", args.data.data_len);
    else printf("Upload failed\n");

    free(args.data.data_val);
}

void download_file(void) {
    char *server_name_ptr;
    char server_name[256];
    char local_path[512];
    DownloadResult *result;
    FILE *fp;

    printf("Enter server filename: ");
    fflush(stdout);
    if (!fgets(server_name, sizeof(server_name), stdin)) return;
    server_name[strcspn(server_name, "\n")] = 0;

    ensure_download_dir();
    snprintf(local_path, sizeof(local_path), "%s/%s", DOWNLOAD_DIR, server_name);

    server_name_ptr = server_name;
    result = download_file_1(&server_name_ptr, clnt);

    if (result == NULL) clnt_perror(clnt, "RPC failed");
    else if (!result->success) printf("File not found\n");
    else {
        fp = fopen(local_path, "wb");
        if (fp) {
            fwrite(result->data.data_val, 1, result->data.data_len, fp);
            fclose(fp);
            printf("Downloaded to %s (%u bytes)\n", local_path, result->data.data_len);
        } else printf("Could not write local file\n");
    }
}

void list_files(void) {
    FileList *result;
    void *dummy = NULL;

    result = list_files_1(dummy, clnt);
    if (result == NULL) clnt_perror(clnt, "RPC failed");
    else if (result->count == 0) printf("No files\n");
    else {
        printf("Files (%d total):\n", result->count);
        char *files = result->files;
        char *token = strtok(files, "\n");
        int i = 1;
        while (token) { printf("  %d. %s\n", i++, token); token = strtok(NULL, "\n"); }
    }
}

void delete_file(void) {
    char *server_name_ptr;
    char server_name[256];
    char confirm[10];
    int *result;

    printf("Enter filename to delete: ");
    fflush(stdout);
    if (!fgets(server_name, sizeof(server_name), stdin)) return;
    server_name[strcspn(server_name, "\n")] = 0;

    printf("Confirm delete? (y/N): ");
    fflush(stdout);
    if (!fgets(confirm, sizeof(confirm), stdin)) return;
    if (strcmp(confirm, "y\n") != 0) { printf("Cancelled\n"); return; }

    server_name_ptr = server_name;
    result = delete_file_1(&server_name_ptr, clnt);
    if (result == NULL) clnt_perror(clnt, "RPC failed");
    else if (*result == 0) printf("Deleted\n");
    else printf("Delete failed\n");
}

int main(int argc, char *argv[]) {
    char choice[10];

    if (argc < 2) {
        printf("Usage: %s <server-host>[:port]\n", argv[0]);
        printf("Examples:\n");
        printf("  %s localhost:8085      (direct port, no portmapper)\n", argv[0]);
        printf("  %s 192.168.1.100       (uses portmapper)\n", argv[0]);
        exit(1);
    }

    printf("Connecting to %s...\n", argv[1]);
    clnt = filemanager_connect(argv[1]);
    if (clnt == NULL) exit(1);
    printf("Connected!\n");

    while (1) {
        printf("\n--- Menu ---\n1. Upload\n2. Download\n3. List\n4. Delete\n0. Exit\nChoice: ");
        fflush(stdout);
        if (!fgets(choice, sizeof(choice), stdin)) break;

        if (strcmp(choice, "1\n") == 0) upload_file();
        else if (strcmp(choice, "2\n") == 0) download_file();
        else if (strcmp(choice, "3\n") == 0) list_files();
        else if (strcmp(choice, "4\n") == 0) delete_file();
        else if (strcmp(choice, "0\n") == 0) break;
    }

    clnt_destroy(clnt);
    printf("Goodbye!\n");
    return 0;
}
