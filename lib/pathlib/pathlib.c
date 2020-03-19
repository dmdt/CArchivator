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

char *getRelativePath(char *base, char *path) {
    unsigned relativePathLength = strlen(path) - strlen(base);
    char *relativePath = (char *)malloc(sizeof(char) * relativePathLength);
    ALLOC_TEST(relativePath);
    strncpy(relativePath, path + strlen(base) + 1, relativePathLength);
    return relativePath;
}

char *getLastEntity(char *path) {
    char *temp = (char *)malloc(sizeof(char)* (strlen(path) + 1));
    strcpy(temp, path);
    char *prev = strtok(temp, DELIMETER_STR);
    char *current = strtok(NULL, DELIMETER_STR);
    while (current != NULL) {
        prev = current;
        current = strtok(NULL, DELIMETER_STR);
    }
    char *lastEntity = (char *)malloc(sizeof(char) * (strlen(prev) + 1));
    strcpy(lastEntity, prev);
    free(temp);
    return lastEntity;
}

int isPathCovered(char *path) {
    unsigned lastSymbol = strlen(path) - 2;
    if (path[lastSymbol] == DELIMETER) {
        return 1;
    }
    return 0;
}

char *stickPath(char *base, char *path) {
    // (+1) - one byte for 0-byte
    unsigned resultSize = strlen(base) + strlen(path) + 1;
    int pathCovered = isPathCovered(base);
    if (pathCovered) {
        resultSize++;
    }
    char *result = (char *) malloc(sizeof(char) * resultSize);
    ALLOC_TEST(result);
    strcpy(result, base);
    if (!pathCovered) {
        strcat(result, DELIMETER_STR);
    }
    strcat(result, path);
    return result;
}