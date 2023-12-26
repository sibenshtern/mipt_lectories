#ifndef UTILS_H
#define UTILS_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <zlib.h>

#include "log.h"

enum { PATH_LENGTH_MAX = 4096, MESSAGE_LENGTH_MAX = 4096 };

enum {
    ERROR_ARGUMENTS = 1,
    ERROR_STAT = 2,
    ERROR_OPEN_SRC = 3,
    ERROR_OPEN_DST = 4,
    ERROR_FILE_DOESNT_EXIST = 6,
    ERROR_FILE_IS_NOT_REG = 13,
    NOT_EQUAL = 14
};

struct config {
    FILE *log_file;
};

typedef struct config config_t;

int check_arguments(int args_count, char **args, config_t *config);
int check_extension(char *filename, char *extension);
int check_file(char *filename, int is_reg, int is_dir, config_t *config);

int compare_files(char *src, char *dst, config_t *config);
int compress_file(char *file, config_t *config);

#endif  // UTILS_H