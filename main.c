#include <stdio.h>
#include <dirent.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/stat.h>

#include "lib/pathlib/pathlib.h"

#ifndef DEBUG
#define PRINTD(...) printf(__VA_ARGS__);
#else
#define PRINTD(...)
#endif

#define ALLOC_TEST(x) \
if (!x) {\
    printf("Allocation error.\n");\
    exit(1);\
}

#define CHUNK_SIZE 8192

int *archive = NULL;

typedef struct {
    unsigned size;
    char *path;
} record;

record *prepareFile(char *path, char *relPath) {
    //Struct with fileMeta meta
    record *fileMeta = (record *) malloc(sizeof(record));
    ALLOC_TEST(fileMeta)
    int file = open(path, O_RDONLY);
    if (file == -1) {
        PRINTD("Error opening fileMeta %s.", path)
        fileMeta->path = NULL;
        return fileMeta;
    }
    fileMeta->size = lseek(file, 0, SEEK_END);
    fileMeta->path = relPath;
    PRINTD("File size %u Bytes\n", fileMeta->size)
    PRINTD("File relative path: %s\n", fileMeta->path)
    close(file);
    return fileMeta;
}

void packFile(record *fileMeta, char *file) {
    write(*archive, &(fileMeta->size), sizeof(unsigned));
    write(*archive, fileMeta->path, strlen(fileMeta->path) + 1);
    char *chunk = (char *) malloc(sizeof(char) * CHUNK_SIZE);
    ALLOC_TEST(chunk)
    int sourceFile = open(file, O_RDONLY);
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
    ALLOC_TEST(result)
    strcpy(result, first);
    strcat(result, second);
    return result;
}

//Error handling?
void createArchive(char *path, char *filename) {
    //Check file existance
    char *incompletePath = stickPath(path, filename);
    char *completePath = concatenate(incompletePath, ".arch");
    int iArchive = open(completePath, O_RDONLY);
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
    PRINTD("New Path %s\n", completePath)
    int arch = open(completePath, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (arch == -1) {
        printf("Error opening file.\n");
        return;
    }
    archive = (int *) malloc(sizeof(int));
    ALLOC_TEST(archive)
    *archive = arch;
    free(completePath);
    free(incompletePath);
}

void closeArchive() {
    close(*archive);
    free(archive);
    archive = NULL;
}

void roundDirectory(char *origin_path, char *path) {
    DIR *dir = opendir(path);
    struct dirent *entry = readdir(dir);
    while (entry != NULL) {
        if (strcmp(entry->d_name, ".") != 0 && strcmp(entry->d_name, "..") != 0) {
            if (entry->d_type == DT_DIR) {
                PRINTD("Folder %s\n", entry->d_name)
                char *new_dir = stickPath(path, entry->d_name);
                PRINTD("---->\n")
                roundDirectory(origin_path, new_dir);
                PRINTD("<----\n")
                free(new_dir);
            } else {
                PRINTD("File %s\n", entry->d_name)
                char *file = stickPath(path, entry->d_name);
                char *fileRelativePath = getRelativePath(origin_path, file);
                record *fileMeta = prepareFile(file, fileRelativePath);
                if (fileMeta->path == NULL) {
                    PRINTD("Can't archive file %s, skipping.", path)
                } else {
                    packFile(fileMeta, file);
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

//FIXME: refactor
char *createResultFolder(char *path, char *name) {
    char *folderPath = stickPath(path, name);
    char *attemptedPath = folderPath;

    int isPathExists = checkPath(folderPath);

    if (isPathExists) {
        int counter = 1;
        char *attemptCounter;
        while (1) {
            attemptCounter = numToString(counter);
            attemptedPath = concatenate(folderPath, attemptCounter);
            free(attemptCounter);
            isPathExists = checkPath(attemptedPath);
            if (!isPathExists) {
                break;
            }
            free(attemptedPath);
            counter++;
        }
    }
    createFolder(attemptedPath);
    if (attemptedPath == folderPath) {
        return folderPath;
    }
    free(folderPath);
    return attemptedPath;
}

int openArchive(char *path) {
    int arch = open(path, O_RDONLY);
    if (arch == -1) {
        return 0;
    }
    archive = (int *) malloc(sizeof(int));
    ALLOC_TEST(archive)
    *archive = arch;
    return 1;
}

int readMeta(record *fileMeta) {
    unsigned sizeTemp;
    char *pathTemp = (char *) malloc(sizeof(char) * 256);
    ALLOC_TEST(pathTemp)
    if (read(*archive, &sizeTemp, sizeof(unsigned)) == 0) {
        free(pathTemp);
        return 0;
    }
    char temp = ' ';
    for (int i = 0; temp != '\0'; i++) {
        if (read(*archive, &temp, 1) == 0) {
            return 0;
        }
        pathTemp[i] = temp;
    }
    fileMeta->size = sizeTemp;
    fileMeta->path = pathTemp;
    return 1;
}

void printMeta(record *fileMeta) {
    printf("File \"%s\", size %u bytes.\n", fileMeta->path, fileMeta->size);
}


int unpackFile(char* path, unsigned length) {
    //Number of restored bytes
    int bytes = 0;
    char *chunk = (char *) malloc(sizeof(char) * CHUNK_SIZE);
    ALLOC_TEST(chunk)
    int file = open(path, O_WRONLY | O_CREAT | O_EXCL, S_IRUSR | S_IWUSR);
    if (file == -1) {
        PRINTD("Error creating file %s.\n", path)
    }
    //Write file
    for (int i = 0; i < length / CHUNK_SIZE; i++) {
        read(*archive, chunk, CHUNK_SIZE);
        bytes += write(file, chunk, CHUNK_SIZE);
    }
    read(*archive, chunk, length % CHUNK_SIZE);
    bytes += write(file, chunk, length % CHUNK_SIZE);
    close(file);
    free(chunk);
    return bytes;
}

void unpackArchive(char *path) {
    char *resultPath = NULL;
    char *innerPath = NULL;
    char *fileName = NULL;
    char *fullPath = NULL;
    record *fileMeta = (record *) malloc(sizeof(record));
    ALLOC_TEST(fileMeta)
    //Number of restored bytes
    int bytes = 0;
    //Number of extracted files
    int files = 0;
    while (readMeta(fileMeta)) {
        printf("Unpacking %s...\n", fileMeta->path);
        fileName = getLastEntity(fileMeta->path);
        if (isFilename(fileMeta->path)) {
            resultPath = copyString(path);
        } else {
            innerPath = getPathToFile(fileMeta->path, fileName);
            resultPath = stickPath(path, innerPath);
        }
        if (!checkPath(resultPath)) {
            createPath(path, innerPath);
        }
        fullPath = stickPath(resultPath, fileName);
        bytes += unpackFile(fullPath, fileMeta->size);
        if (fileMeta->path != NULL) {
            free(fileMeta->path);
        }
        free(fileName);
        free(innerPath);
        free(resultPath);
        free(fullPath);
        files++;
    }
    PRINTD("Restored %d bytes in %d files.\n", bytes, files)
    free(fileMeta);
}

int main(int argc, char **argv) {
    char *path = NULL;
    char *name = NULL;
    char *destinationPath = NULL;
    //Mode "1" - unpack, mode "2" - pack, mode "3" - show archive content
    int mode = 0;
    // Parse arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(*(argv + i), "-m") == 0) {
            i++;
            if (strcmp(*(argv + i), "u") == 0) {
                mode = 1;
            } else if (strcmp(*(argv + i), "p") == 0) {
                mode = 2;
            } else if (strcmp(*(argv + i), "s") == 0) {
                mode = 3;
            }
        } else if (strcmp(*(argv + i), "-p") == 0) {
            i++;
            path = *(argv + i);
        } else if (strcmp(*(argv + i), "-d") == 0) {
            i++;
            destinationPath = *(argv + i);
        } else if (strcmp(*(argv + i), "-n") == 0) {
            i++;
            name = *(argv + i);
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
    char *lastPathDir = getLastEntity(path);
    //Check is name for archive were specified
    if (name == NULL) {
        if (lastPathDir != NULL) {
            if (isFilename(lastPathDir)) {
                name = cutExtension(lastPathDir);
            } else {
                name = lastPathDir;
            }
        } else {
            printf("Incorrect filename/path specified.");
            return 1;
        }
    }
    if (mode < 3) {
        // Is destination path were specified
        if (destinationPath == NULL) {
            destinationPath = getPathToFile(path, lastPathDir);
            PRINTD("Result folder: %s\n", destinationPath)
        }
        //Check destination path
        if (path != destinationPath) {
            if (!checkPath(destinationPath)) {
                printf("Incorrect destination path: %s\n", destinationPath);
                return 1;
            }
        }
    }
    //Mode selection
    if (mode == 1) {
        if (!openArchive(path)) {
            printf("Error opening archive.\n");
            return 1;
        }
        char *resFolder = createResultFolder(destinationPath, name);
        unpackArchive(resFolder);
        closeArchive();
    } else if (mode == 2) {
        //Check path availability
        if (!checkPath(path)) {
            printf("Incorrect path: %s\n", path);
            return 1;
        }
        createArchive(destinationPath, name);
        roundDirectory(path, path);
        closeArchive();
    } else {
        int files = 0;
        record *fileMeta = (record *) malloc(sizeof(record));
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
            files++;
        }
        free(fileMeta);
        printf("Archive size %ld bytes. %d files.\n", lseek(*archive, 0, SEEK_CUR), files);
        closeArchive();
    }
    free(lastPathDir);
    return 0;
}
