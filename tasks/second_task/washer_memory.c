#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <fcntl.h>

#include "common.h"

#include <sys/shm.h>
int send_dish(dish_t *dish, char **memory, size_t *position);
size_t N = 0;

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

    key_t key = ftok("test.txt", 0);
    if (key < 0) {
        fprintf(stderr, "Can't find file test.txt in directory");
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }

    int shmid = shmget(key, 1024 * N, IPC_CREAT | 0666);
    if (shmid < 0) {
        fprintf(stderr, "Can't get shared memory id: %s", strerror(errno));
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }

    void *ptr = shmat(shmid, NULL, 0);
    if (ptr == (void *) -1) {
        fprintf(stderr, "Can't get shared memory: %s", strerror(errno));
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }
    char *memory = (char *) ptr;
    size_t position = 0;

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
            send_dish(&dish, &memory, &position);
            ++table_size;
        }
    }
    fclose(file);
    dish.type = "";
    dish.time = -1;
    send_dish(&dish, &memory, &position);
    shmdt(memory);

    char *status = NULL;
    size_t size = 0;
    while (read_pipe(&status, '\0', &size, status_fd) > 0 && strcmp(status, "-1") != 0)
        continue;
    close(status_fd);
    return 0;
}

int send_dish(dish_t *dish, char **memory, size_t *position) {
    size_t size = strlen(dish->type) + 1;
    strcpy((*memory + *position), dish->type);
    *position += size;
    return 0;
}