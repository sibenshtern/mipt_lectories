#include "utils.h"

int check_arguments(int args_count, char **args, config_t *config) {
    if (args_count == 2 && strcmp(args[1], "--help") == 0) {
        printf("Usage: %s [SOURCE DIRECTORY] [DESTINATION DIRECTORY] (-z)\n",
               args[0]);
        printf("An intelligent program for creating backups of your files\n");

        printf("Options:\n");
        printf("\t--help\tdisplay this help and exit\n");
        return 0;
    } else if (args_count < 3) {
        printf("Too few arguments. Use '%s --help' to see usage\n", args[0]);
        return ERROR_ARGUMENTS;
    }

    struct stat src;
    struct stat dst;

    if (lstat(args[1], &src) != 0) {
        // log message
        char message[MESSAGE_LENGTH_MAX];
        sprintf(message, "check_arguments: source directory error: %s",
                strerror(errno));
        logging(config->log_file, 'E', message);

        return ERROR_STAT;
    }
    if (stat(args[2], &dst) != 0) {
        mkdir(args[2], S_IRWXU | S_IRWXG | S_IRWXO);
        return 0;
    }

    if (!S_ISDIR(src.st_mode)) {
        logging(config->log_file, 'E',
                "backup_directory: source path is not directory\n");
        fprintf(stderr, "backup_directory: source path is not directory\n");
        return ERROR_ARGUMENTS;
    }
    if (!S_ISDIR(dst.st_mode)) {
        logging(config->log_file, 'E',
                "backup_directory: destination path is not directory\n");
        fprintf(stderr,
                "backup_directory: destination path is not directory\n");
        return ERROR_ARGUMENTS;
    }
    return 0;
}

int check_file(char *filename, int is_reg, int is_dir, config_t *config) {
    struct stat info;
    if (lstat(filename, &info)) {
        char message[MESSAGE_LENGTH_MAX];
        sprintf(message, "check_file: can't open '%s' file\n", filename);
        logging(config->log_file, 'W', message);

        return ERROR_STAT;
    }

    return (S_ISREG(info.st_mode) && is_reg) ||
           (S_ISDIR(info.st_mode) && is_dir);
}

int check_extension(char *filename, char *extension) {
    size_t extension_length = strlen(extension);
    return (strlen(filename) > extension_length &&
            strcmp(filename + strlen(filename) - extension_length, extension) ==
                0);
}

int compress_file(char *filepath, config_t *config) {
    if (check_file(filepath, 1, 0, config) == 0) {
        fprintf(stderr, "compress_file: '%s' file is not regular\n", filepath);
        return ERROR_FILE_IS_NOT_REG;
    }

    char *archive_filepath = malloc(strlen(filepath) + 4);
    sprintf(archive_filepath, "%s.gz", filepath);

    gzFile archive = gzopen(archive_filepath, "wb");

    // read file
    int file_fd = open(filepath, O_RDONLY, 0666);
    size_t file_size = lseek(file_fd, 0, SEEK_END);
    lseek(file_fd, 0, SEEK_SET);

    size_t buffer_size = 512;
    char buffer[buffer_size];

    size_t read_count = 0;
    size_t sum_read = 0;

    // write to archive
    while ((read_count = read(file_fd, buffer, buffer_size * sizeof(char))) ==
           buffer_size) {
        gzwrite(archive, buffer, read_count);
        sum_read += read_count;
    }

    gzwrite(archive, buffer, read_count);
    sum_read += read_count;

    if (sum_read != file_size) {
        fprintf(stderr, "compress_file: can't read all file");
    }

    gzclose(archive);
    remove(filepath);
    return 0;
}

int compare_files(char *src, char *dst, config_t *config) {
    struct stat src_info;
    struct stat dst_info;

    if (lstat(src, &src_info) == -1) {
        // log message
        char message[MESSAGE_LENGTH_MAX];
        sprintf(message, "compare_files: can't open source file '%s'\n", src);
        logging(config->log_file, 'E', message);
        fprintf(stderr, "%s", message);
        return ERROR_STAT;
    }

    char *archive_path = malloc(strlen(dst) + 4);
    sprintf(archive_path, "%s.gz", dst);

    if (lstat(archive_path, &dst_info) == -1) {
        // log message
        char message[MESSAGE_LENGTH_MAX];
        sprintf(message, "compare_files: %s doesn't exist", dst);
        logging(config->log_file, 'D', message);

        free(archive_path);
        return ERROR_FILE_DOESNT_EXIST;
    }

    if (src_info.st_mtime > dst_info.st_mtime) {
        free(archive_path);
        return NOT_EQUAL;
    }
    free(archive_path);
    return 0;
}
