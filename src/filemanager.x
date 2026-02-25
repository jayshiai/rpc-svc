const MAX_FILENAME_LEN = 256;
const MAX_DATA_LEN = 10485760;
const MAX_FILES = 100;

struct UploadArgs {
    string filename<MAX_FILENAME_LEN>;
    opaque data<MAX_DATA_LEN>;
};

struct DownloadResult {
    int success;
    opaque data<MAX_DATA_LEN>;
};

struct FileList {
    int count;
    string files<25600>;
};

program FILEMANAGER_PROG {
    version FILEMANAGER_VERS {
        int UPLOAD_FILE(UploadArgs) = 1;
        DownloadResult DOWNLOAD_FILE(string) = 2;
        FileList LIST_FILES(void) = 3;
        int DELETE_FILE(string) = 4;
    } = 1;
} = 0x20000001;
