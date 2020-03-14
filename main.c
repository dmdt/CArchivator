#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#ifndef DEBUG
#define PRINTD(...) printf(__VA_ARGS__);
#else
#define PRINTD(...)
#endif

int *archive = NULL;

typedef struct {
    unsigned size;
    char *path;
} record;

char *getRelativePath(char *root_path, char *file_path) {
    unsigned relativePathLen = strlen(file_path) - strlen(root_path);
    char *relativePath = (char *) malloc(sizeof(char) * relativePathLen);
    strncpy(relativePath, file_path + strlen(root_path) + 1, relativePathLen);
    return relativePath;
}

void archive_file(char *path, char *relPath) {
    record *file = (record *) malloc(sizeof(record));
    int file2read = open(path, O_RDONLY);
    if (file2read == -1) {
        PRINTD("Error opening file %s.", path);
        return;
    }
    lseek(file2read, 0, SEEK_END);
    file->size = tell(file2read);
    file->path = relPath;
    PRINTD("File size %d Bytes\n", file->size);
    PRINTD("File relative path: %s\n", file->path);
    close(file2read);
    free(file);
}

char *getNumString(int num) {
    int size = 1;
    int temp = num;
    do {
        size++;
        temp /= 10;
    } while (temp > 0);
    char *number = (char *) malloc(sizeof(char) * size);
    for (int i = size - 2; i > -1; i--) {
        number[i] = (num % 10) + '0';
        num /= 10;
    }
    number[size - 1] = '\0';
    return number;
}

char *stick_path(char *base, char *filename) {
    unsigned result_size = strlen(base) + strlen(filename) + 2;
    char *result = (char *) malloc(sizeof(char) * result_size);
    strcpy(result, base);
    strcat(result, "\\");
    strcat(result, filename);
    return result;
}

char *concatenate(char *first, char *second) {
    unsigned result_size = strlen(first) + strlen(second) + 1;
    char *result = (char *) malloc(sizeof(char) * result_size);
    strcpy(result, first);
    strcat(result, second);
    return result;
}

char *getLastDir(char *path) {
    char *tempPath = (char *) malloc(sizeof(char) * strlen(path) + 1);
    strcpy(tempPath, path);
    char *prev = strtok(tempPath, "\\/");
    char *current = strtok(NULL, "\\/");
    while (current != NULL) {
        prev = current;
        current = strtok(NULL, "\\/");
    }
    char *last_dir = (char *) malloc(sizeof(char) * strlen(prev) + 1);
    strcpy(last_dir, prev);
    free(tempPath);
    return last_dir;
}

void createArchive(char *path) {
    // Get filename for archive
    char *filename = getLastDir(path);
    PRINTD("Archive name: %s\n", filename);
    if (filename == NULL) {
        printf("Incorrect file path.\n");
        return;
    }
    // Get path where archive will be placed
    unsigned folder_len = strlen(path) - strlen(filename);
    char *folder = (char *) malloc(sizeof(char) * folder_len);
    strncpy(folder, path, folder_len);
    folder[folder_len - 1] = '\0';
    PRINTD("Result folder: %s\n", folder);
    //Check file existance
    char *incompletePath = stick_path(folder, filename);
    char *completePath = concatenate(incompletePath, ".arch");
    int iArchive = open(completePath, O_RDONLY);
    if (iArchive > 0) {
        int attempt = 0;
        do {
            close(iArchive);
            attempt++;
            free(completePath);
            char *iterationString = getNumString(attempt);
            char *pathWithIteration = concatenate(incompletePath, iterationString);
            completePath = concatenate(pathWithIteration, ".arch");
            free(pathWithIteration);
            free(iterationString);
            iArchive = open(completePath, O_RDONLY);
        } while (iArchive > 0);
    }
    PRINTD("New Path %s\n", completePath);
    int arch = open(completePath, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (arch == -1) {
        printf("Error opening file.\n");
        return;
    }
    archive = (int *) malloc(sizeof(int));
    *archive = arch;
    free(folder);
    free(filename);
    free(completePath);
    free(incompletePath);
}

void list_directories(char *origin_path, char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry = readdir(dir);
    while (entry != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if (entry->d_type == DT_DIR) {
                printf("Folder %s\n", entry->d_name);
                char *new_dir = stick_path(path, entry->d_name);
                PRINTD("---->\n");
                list_directories(origin_path, new_dir);
                PRINTD("<----\n");
                free(new_dir);
            } else {
                printf("File %s\n", entry->d_name);
                char *file = stick_path(path, entry->d_name);
                archive_file(file, getRelativePath(origin_path, file));
                free(file);
            }
        }
        entry = readdir(dir);
    }
    closedir(dir);
}

int main() {
    char path[] = "C:\\Users\\dmtprs\\Desktop\\New folder";
    createArchive(path);
//    int file = open("C:\\Users\\dmtprs\\Desktop\\New folder.arch", O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
//    printf("%d", file);
//    list_directories(path, path);
    return 0;
}
