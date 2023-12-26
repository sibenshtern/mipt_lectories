#ifndef BACKUP_H
#define BACKUP_H

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>

#include "utils.h"

enum { ERROR_OPEN_DIRECTORY = 7 };

int copy_file(char *src, char *dst, config_t *config);
int backup_directory(char *src, char *dst, config_t *config);
int check_backup(char *src, char *dst, config_t *config);

#endif  // BACKUP_H