#ifndef WEFT_FILE_H
#define WEFT_FILE_H

#include <stdio.h>

// Functions

FILE *file_open(const char *path, const char *mode);
FILE *file_open_n(const char *path, size_t path_len, const char *mode);
void file_close(FILE *f);
size_t file_len(FILE *f);

#endif
