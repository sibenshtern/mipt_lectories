#include "backup.h"

int copy_file(char *src, char *dst, config_t *config) {
    int src_fd = open(src, O_RDONLY, 0666);
    if (src_fd == -1) {
        return ERROR_OPEN_SRC;
    }

    int dst_fd = open(dst, O_CREAT | O_WRONLY | O_TRUNC, 0666);
    if (dst_fd == -1) {
        close(src_fd);
        return ERROR_OPEN_DST;
    }

    size_t file_size = lseek(src_fd, 0, SEEK_END);
    lseek(src_fd, 0, SEEK_SET);

    size_t write_count = sendfile(dst_fd, src_fd, NULL, file_size);
    if (write_count != file_size) {
        // log message
        char message[MESSAGE_LENGTH_MAX];
        sprintf(message, "copy_file: can't write all file\n");
        logging(config->log_file, 'W', message);
    }

    close(dst_fd);
    close(src_fd);

    compress_file(dst, config);
    return 0;
}

int backup_directory(char *src, char *dst, config_t *config) {
    DIR *src_dir = opendir(src);
    if (src_dir == NULL) {
        // log message
        char message[MESSAGE_LENGTH_MAX];
        sprintf(message, "backup_directory: can't open source directory '%s'\n",
                src);
        logging(config->log_file, 'E', message);

        fprintf(stderr, "%s", message);
        return ERROR_OPEN_DIRECTORY;
    }

    struct dirent *dp;
    struct stat src_stat;

    while ((dp = readdir(src_dir)) != NULL) {
        char src_path[PATH_LENGTH_MAX + 1];
        char dst_path[PATH_LENGTH_MAX + 1];
        char archive_path[PATH_LENGTH_MAX + 1];

        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }

        sprintf(src_path, "%s/%s", src, dp->d_name);
        sprintf(dst_path, "%s/%s", dst, dp->d_name);

        lstat(src_path, &src_stat);

        if (S_ISDIR(src_stat.st_mode)) {
            mkdir(dst_path, S_IRWXU | S_IRWXG | S_IRWXO);
            backup_directory(src_path, dst_path, config);
        } else if (S_ISREG(src_stat.st_mode)) {
            sprintf(archive_path, "%s/%s.gz", dst, dp->d_name);

            int compare_result = compare_files(src_path, dst_path, config);
            if (compare_result != 0 && compare_result != ERROR_STAT) {
                copy_file(src_path, dst_path, config);
            }
        }
    }
    closedir(src_dir);
    return 0;
}

int check_backup(char *src, char *dst, config_t *config) {
    DIR *dir = opendir(dst);
    if (dir == NULL) {
        // log message
        char message[MESSAGE_LENGTH_MAX];
        sprintf(message, "check_backup: can't open directory '%s'\n", dst);
        logging(config->log_file, 'E', message);

        fprintf(stderr, "%s", message);
        return ERROR_OPEN_DIRECTORY;
    }

    struct dirent *dp;
    struct stat src_info;

    while ((dp = readdir(dir)) != NULL) {
        char temp_path[PATH_LENGTH_MAX + 1];
        char src_path[PATH_LENGTH_MAX + 1];
        char dst_path[PATH_LENGTH_MAX + 1];

        if (strcmp(dp->d_name, ".") == 0 || strcmp(dp->d_name, "..") == 0) {
            continue;
        }

        sprintf(temp_path, "%s/%s", src, dp->d_name);
        sprintf(dst_path, dst, dp->d_name);

        if (check_extension(dst, ".gz")) {
            strncpy(src_path, temp_path, strlen(temp_path) - 3);
            src_path[strlen(temp_path) - 3] = 0;
        } else {
            strcpy(src_path, temp_path);
        }

        if (lstat(src_path, &src_info) == -1) {
            remove(dst_path);
        } else if (S_ISDIR(src_info.st_mode)) {
            check_backup(src_path, dst_path, config);
        }
    }
    closedir(dir);
    return 0;
}
