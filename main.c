#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/stat.h>

#include "lib/pathlib/pathlib.h"

#ifndef DEBUG
#define PRINTD(...) printf(__VA_ARGS__);
#else
#define PRINTD(...)
#endif

#define CHUNK_SIZE 8192

int *archive = NULL;

typedef struct {
    unsigned size;
    char *path;
} record;

record *prepareFile(char *path, char *relPath) {
    //Struct with file meta
    record *file = (record *) malloc(sizeof(record));
    int file2read = open(path, O_RDONLY | O_BINARY);
    if (file2read == -1) {
        PRINTD("Error opening file %s.", path);
        file->path = NULL;
        return file;
    }
    lseek(file2read, 0, SEEK_END);
    file->size = tell(file2read);
    file->path = relPath;
    PRINTD("File size %d Bytes\n", file->size);
    PRINTD("File relative path: %s\n", file->path);
    close(file2read);
    return file;
}

void archiveFile(record *fileMeta, char *file) {
    write(*archive, &(fileMeta->size), sizeof(unsigned));
    write(*archive, fileMeta->path, strlen(fileMeta->path) + 1);
    char *chunk = (char*)malloc(sizeof(char) * CHUNK_SIZE);
    int sourceFile = open(file, O_RDONLY | O_BINARY);
    for (unsigned i = 0; i < fileMeta->size / CHUNK_SIZE; i++) {
        read(sourceFile, chunk, CHUNK_SIZE);
        write(*archive, chunk, CHUNK_SIZE);
    }
    read(sourceFile, chunk, (fileMeta->size % CHUNK_SIZE));
    write(*archive, chunk, (fileMeta->size % CHUNK_SIZE));
    free(chunk);
    close(sourceFile);
}

char *numToString(int num) {
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

char *concatenate(char *first, char *second) {
    unsigned result_size = strlen(first) + strlen(second) + 1;
    char *result = (char *) malloc(sizeof(char) * result_size);
    strcpy(result, first);
    strcat(result, second);
    return result;
}

//Error handling?
void createArchive(char *path, char* filename) {
    //Check file existance
    char *incompletePath = stickPath(path, filename);
    char *completePath = concatenate(incompletePath, ".arch");
    int iArchive = open(completePath, O_RDONLY | O_BINARY);
    if (iArchive > 0) {
        int attempt = 0;
        do {
            close(iArchive);
            attempt++;
            free(completePath);
            char *iterationString = numToString(attempt);
            char *pathWithIteration = concatenate(incompletePath, iterationString);
            completePath = concatenate(pathWithIteration, ".arch");
            free(pathWithIteration);
            free(iterationString);
            iArchive = open(completePath, O_RDONLY);
        } while (iArchive > 0);
    }
    PRINTD("New Path %s\n", completePath);
    int arch = open(completePath, O_WRONLY | O_CREAT | O_EXCL | O_BINARY, S_IRUSR | S_IWUSR);
    if (arch == -1) {
        printf("Error opening file.\n");
        return;
    }
    archive = (int *) malloc(sizeof(int));
    *archive = arch;
    free(completePath);
    free(incompletePath);
}

void closeArchive() {
    close(*archive);
    free(archive);
    archive = NULL;
}

void list_directories(char *origin_path, char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry = readdir(dir);
    while (entry != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if (entry->d_type == DT_DIR) {
                PRINTD("Folder %s\n", entry->d_name);
                char *new_dir = stickPath(path, entry->d_name);
                PRINTD("---->\n");
                list_directories(origin_path, new_dir);
                PRINTD("<----\n");
                free(new_dir);
            } else {
                PRINTD("File %s\n", entry->d_name);
                char *file = stickPath(path, entry->d_name);
                char *fileRelativePath = getRelativePath(origin_path, file);
                record *fileMeta = prepareFile(file, fileRelativePath);
                if (fileMeta->path == NULL) {
                    PRINTD("Can't archive file %s, skipping.", path);
                } else {
                    archiveFile(fileMeta, file);
                }
                free(file);
                free(fileMeta);
                free(fileRelativePath);
            }
        }
        entry = readdir(dir);
    }
    closedir(dir);
}

void createFolder(char* path, char* name) {

}

int checkPath(char* path) {
    DIR *dir = opendir(path);
    if (dir == NULL) {
        closedir(dir);
        return 0;
    }
    closedir(dir);
    return 1;
}

int openArchive(char* path) {
    int arch = open(path, O_RDONLY | O_BINARY);
    if (arch == -1) {
        return 0;
    }
    archive = (int *) malloc(sizeof(int));
    *archive = arch;
    return 1;
}

int readMeta(record* fileMeta) {
    unsigned sizeTemp;
    char* pathTemp = (char*)malloc(sizeof(char)*256);
    if (read(*archive, &sizeTemp, sizeof(unsigned)) == 0) {
        return 0;
    }
    char temp = ' ';
    for (int i = 0; temp != '\0' ; i++) {
        if (read(*archive, &temp, 1) == 0) {
            return 0;
        }
        pathTemp[i] = temp;
    }
    fileMeta->size = sizeTemp;
    fileMeta->path = pathTemp;
    return 1;
}

void printMeta(record* fileMeta) {
    printf("File \"%s\", size %d bytes.\n", fileMeta->path, fileMeta->size);
}

int main(int argc, char** argv) {
    char* path = NULL;
    char* name = NULL;
    char* destinationPath = NULL;
    //Mode "1" - unpack, mode "2" - pack, mode "3" - show archive content
    int mode = 0;
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(*(argv+i), "-m") == 0) {
            i++;
            if (strcmp(*(argv+i), "u") == 0) {
                mode = 1;
            } else if (strcmp(*(argv+i), "p") == 0) {
                mode = 2;
            } else if (strcmp(*(argv+i), "s") == 0) {
                mode = 3;
            }
        } else if (strcmp(*(argv+i), "-p") == 0) {
            i++;
            path = *(argv+i);
        } else if (strcmp(*(argv+i), "-d") == 0) {
            i++;
            destinationPath = *(argv+i);
        } else if (strcmp(*(argv+i), "-n") == 0) {
            i++;
            name = *(argv+i);
        }
    }

    // Is mode were selected
    if (!mode) {
        printf("Select action [-m u/p/s]\n");
        return 1;
    }
    // Is path were specified
    if (path == NULL) {
        printf("Select path [-p \"path\"]\n");
        return 1;
    }
    //Cut last folder name from path
    char* lastPathDir = getLastEntity(path);
    if (mode < 3) {
        // Is destination path were specified
        if (destinationPath == NULL) {
            unsigned folder_len = strlen(path) - strlen(lastPathDir);
            destinationPath = (char*) malloc(sizeof(char) * folder_len);
            strncpy(destinationPath, path, folder_len);
            destinationPath[folder_len - 1] = '\0';
            PRINTD("Result folder: %s\n", destinationPath);
        }
        //Check path availability
        if (!checkPath(path)) {
            printf("Incorrect path: %s\n", path);
            return 1;
        }
        //Check destination path
        if (path != destinationPath) {
            if (!checkPath(destinationPath)) {
                printf("Incorrect destination path: %s\n", destinationPath);
                return 1;
            }
        }
        //Check is name for archive were specified
        if (name == NULL) {
            name = lastPathDir;
            if (name == NULL) {
                printf("Incorrect filename/path specified.");
                return 1;
            }
        }
    }
    //
    if (mode == 1) {
        createFolder(destinationPath, name);
    } else if (mode == 2) {
        createArchive(destinationPath, name);
        list_directories(path, path);
        closeArchive();
    } else {
        record* fileMeta = (record*)malloc(sizeof(record));
        if (!openArchive(path)) {
            printf("Error opening archive.\n");
            return 1;
        }
        printf("Archive content:\n");
        while (readMeta(fileMeta)) {
            lseek(*archive, fileMeta->size, SEEK_CUR);
            printMeta(fileMeta);
            if (fileMeta->path != NULL) {
                free(fileMeta->path);
            }
        }
        free(fileMeta);
        closeArchive();
    }
    free(lastPathDir);
    return 0;
}
