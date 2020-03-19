/* This library provide convenient way working with paths.
 *
 *
 **/

#ifndef PATHLIB_H
#define PATHLIB_H

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

char *getRelativePath(char *, char *);

char *stickPath(char *, char *);

char *getLastEntity(char *);

int isPathCovered(char *);

#endif
