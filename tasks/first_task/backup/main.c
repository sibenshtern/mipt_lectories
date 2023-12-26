#include <time.h>

#include "backup.h"
#include "utils.h"

int main(int argc, char **argv) {
    // default configuration
    config_t config;
    FILE *log_file = fopen("logging.log", "w");
    config.log_file = log_file;

    // checking arguments
    int result = check_arguments(argc, argv, &config);
    if (result != 0) {
        return result;
    }

    char src[PATH_LENGTH_MAX + 1];
    char dst[PATH_LENGTH_MAX + 1];

    strcpy(src, argv[1]);
    strcpy(dst, argv[2]);

    double time_spent = 0.0;
    clock_t begin = clock();
    result = backup_directory(src, dst, &config);
    check_backup(src, dst, &config);
    clock_t end = clock();

    time_spent += (double)(end - begin) / CLOCKS_PER_SEC;
    printf("Backup all files in %lf seconds\n", time_spent);
    return result;
}
