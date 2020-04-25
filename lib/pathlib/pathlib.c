#include "pathlib.h"

#define ALLOC_TEST(x) \
if (!x) {\
    printf("Allocation error.\n");\
    exit(1);\
}

#define SYSTEM WIN
#if SYSTEM == WIN
#define DELIMETER '\\'
#define DELIMETER_STR "\\"
#else
#define DELIMETER '/'
#define DELIMETER_STR "/"
#endif

char *copyString(const char * str) {
    char * result = (char *) malloc(sizeof(char) * (strlen(str) + 1));
    ALLOC_TEST(result)
    strcpy(result, str);
    return result;
}

char *getRelativePath(char *base, char *path) {
    unsigned relativePathLength = strlen(path) - strlen(base);
    char *relativePath = (char *) malloc(sizeof(char) * relativePathLength);
    ALLOC_TEST(relativePath)
    strncpy(relativePath, path + strlen(base) + 1, relativePathLength);
    return relativePath;
}

char *getLastEntity(char *path) {
    char *temp = (char *) malloc(sizeof(char) * (strlen(path) + 1));
    ALLOC_TEST(temp)
    strcpy(temp, path);
    char *prev = strtok(temp, DELIMETER_STR);
    char *current = strtok(NULL, DELIMETER_STR);
    while (current != NULL) {
        prev = current;
        current = strtok(NULL, DELIMETER_STR);
    }
    char *lastEntity = (char *) malloc(sizeof(char) * (strlen(prev) + 1));
    ALLOC_TEST(lastEntity)
    strcpy(lastEntity, prev);
    free(temp);
    return lastEntity;
}

int isPathCovered(const char *path) {
    unsigned lastSymbol = strlen(path) - 2;
    if (path[lastSymbol] == DELIMETER) {
        return 1;
    }
    return 0;
}

char *stickPath(const char *base, const char *path) {
    // (+1) - one byte for 0-byte
    unsigned resultSize = strlen(base) + strlen(path) + 1;
    int pathCovered = isPathCovered(base);
    if (!pathCovered) {
        resultSize++;
    }
    char *result = (char *) malloc(sizeof(char) * resultSize);
    ALLOC_TEST(result)
    strcpy(result, base);
    if (!pathCovered) {
        strcat(result, DELIMETER_STR);
    }
    strcat(result, path);
    return result;
}

int isFilename(char *name) {
    if (strstr(name, ".")) {
        if (!strstr(name, DELIMETER_STR)) {
            return 1;
        }
    }
    return 0;
}

int checkPath(char *path) {
    DIR *directory = opendir(path);
    if (directory == NULL) {
        closedir(directory);
        return 0;
    }
    closedir(directory);
    return 1;
}

int createFolder(char *path) {
    if (mkdir(path) == -1) {
        return 0;
    }
    return 1;
}

char *cutExtension(char *path) {
    char *dot = strstr(path, ".");
    if (dot == NULL) {
        return NULL;
    }
    unsigned len = strlen(path) - strlen(dot);
    char *result = (char *) malloc(sizeof(char) * (len + 1));
    ALLOC_TEST(result)
    strncpy(result, path, len);
    result[len] = '\0';
    return result;
}

char *getPathToFile(const char *path, const char *filename) {
    unsigned len = strlen(path) - strlen(filename) - 1;
    char *result = (char *) malloc(sizeof(char) * (len + 1));
    ALLOC_TEST(result)
    strncpy(result, path, len);
    result[len] = '\0';
    return result;
}

int decomposePath(const char* path, char *** container) {
    int pathEntities = 1;
    char *findEntities = strstr(path, DELIMETER_STR);
    while (findEntities) {
        pathEntities++;
        findEntities = strstr(findEntities+1, DELIMETER_STR);
    }
    *container = (char **) malloc(sizeof(char *) * pathEntities);
    ALLOC_TEST(*container)
    char *temp = (char *) malloc(sizeof(char) * (strlen(path) + 1));
    ALLOC_TEST(temp)
    strcpy(temp, path);
    char *pathEntity = strtok(temp, DELIMETER_STR);
    for (int i = 0; i < pathEntities; i++) {
        *(*container+i) = copyString(pathEntity);
        pathEntity = strtok(NULL, DELIMETER_STR);
    }
    return pathEntities;
}

void createPath(const char *base, const char *path) {
    char **pathEntities = NULL;
    char *oldPath = copyString(base);
    int entitiesNum = decomposePath(path, &pathEntities);
    char *tempPath = NULL;
    for (int i = 0; i < entitiesNum; i++) {
        tempPath = stickPath(oldPath, *(pathEntities+i));
        if (!checkPath(tempPath)) {
            createFolder(tempPath);
        }
        free(oldPath);
        oldPath = tempPath;
    }
    for (int i = 0; i < entitiesNum; i++) {
        free(*(pathEntities+i));
    }
    free(oldPath);
}