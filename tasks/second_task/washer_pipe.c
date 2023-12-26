#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <fcntl.h>

#include "common.h"

size_t N = 0;

int load_dish(dish_t *dish, FILE *file);
int send_dish(dish_t *dish, int fd);

int main(int argc, char **argv) {
    if (argc != 3) {
        printf("Too few arguments\n");
        return -1;
    }

    char *config_file = argv[1];
    char *file_with_dishes = argv[2];
    char *table_limit = getenv("TABLE_LIMIT");

    if (!table_limit) {
        fprintf(stderr, "No TABLE_LIMIT environment variable set\n");
        return -1;
    }

    char *end = NULL;
    N = strtol(table_limit, &end, 10);
    if (!end) {
        fprintf(stderr, "TABLE_LIMIT environment variable contain something strange thing: '%s'\n", table_limit);
        return -1;
    }

    array_t dishes = {0};
    dishes.capacity = 10;
    dishes.array = calloc(dishes.capacity, sizeof(dish_t));

    int result = load_dishes(&dishes, config_file);
    if (result == -1) {
        free(dishes.array);
        return -1;
    }

    FILE *file = fopen(file_with_dishes, "r");

    if (!file) {
        fprintf(stderr, "Cannot open file '%s'\n", file_with_dishes);
        free(dishes.array);
        return -1;
    }

    size_t table_size = 0;
    int status_fd = open("status.fifo", O_RDONLY);
    if (status_fd < 0) {
        fprintf(stderr, "Can't open 'status.fifo': %s\n", strerror(errno));
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }

    int fd = open("label.fifo", O_WRONLY);
    if (fd < 0) {
        fprintf(stderr, "Can't open 'label.fifo': %s\n", strerror(errno));
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }

    dish_t dish = {0};
    while (load_dish(&dish, file) != -1) {
        size_t time = -1;
        for (size_t i = 0; i < dishes.size; i++) {
            if (strcmp(dishes.array[i].type, dish.type) == 0)
                time = dishes.array[i].time;
        }

        if (time == -1) {
            fprintf(stderr, "Unknown dish type: '%s'\n", dish.type);
            continue;
        }

        for (int i = 0; i < dish.time; i++) {
            printf("Started to wash the plate. It will take %zu seconds\n", time);
            sleep(time);
            printf("Plates on table: %zu\n", table_size);
            while (table_size >= N) {
                char *status = NULL;
                size_t size = 0;
                if (read_pipe(&status, '\0', &size, status_fd) > 0 && strcmp(status, "1") == 0)
                    --table_size;
            }
            send_dish(&dish, fd);
            ++table_size;
        }
    }
    fclose(file);

    char *status = NULL;
    size_t size = 0;
    while (read_pipe(&status, '\0', &size, status_fd) > 0 && strcmp(status, "-1") != 0)
        continue;

    write(fd, "\0", 1);
    close(fd);
    close(status_fd);
    return 0;
}

int send_dish(dish_t *dish, int fd) {
    printf("Washed dish with type: %s\n", dish->type);
    size_t buffer_size = strlen(dish->type) + 1;
    char *buffer = calloc(buffer_size, sizeof(char));
    strcpy(buffer, dish->type);

    size_t written = write(fd, buffer, buffer_size);
    if (written == (size_t) -1) {
        fprintf(stderr, "Could not write the type of dishes in pipe: %s\n",
                strerror(errno));
        return -1;
    }
    return 0;
}