/* This library provide convenient way working with paths.
 *
 *
 **/

#ifndef PATHLIB_H
#define PATHLIB_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <dirent.h>
#include "pathlib.c"

char *getRelativePath(char *, char *);

char *stickPath(const char *, const char *);

char *getLastEntity(char *);

char *cutExtension(char *);

int isPathCovered(const char *);

int checkPath(char *);

int isFilename(char *);

int createFolder(char *);

char *getPathToFile(const char*, const char*);

int decomposePath(const char*, char ***);

void createPath(const char *, const char *);

char *copyString(const char *);

#endif /* PATHLIB_H */
