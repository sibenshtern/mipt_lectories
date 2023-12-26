#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>

#include "common.h"

#include <sys/shm.h>
int get_dish(dish_t *dish, char **memory, size_t *position);

size_t N = 0;

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "Wrong count of arguments\n");
        return EXIT_FAILURE;
    }

    char *config_file = argv[1];
    char *table_limit = getenv("TABLE_LIMIT");

    if (!table_limit) {
        fprintf(stderr, "No TABLE_LIMIT environment variable set\n");
        return EXIT_FAILURE;
    }

    char *end = NULL;
    N = strtol(table_limit, &end, 10);
    if (!end) {
        fprintf(stderr, "TABLE_LIMIT environment variable contain something strange: '%s'\n", end);
        return EXIT_FAILURE;
    }

    array_t table = {0};
    table.array = calloc(N, sizeof(dish_t));
    table.capacity = N;

    array_t dishes = {0};
    dishes.capacity = 10;
    dishes.array = calloc(dishes.capacity, sizeof(dish_t));

    int result = load_dishes(&dishes, config_file);
    if (result == -1) {
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }

    int status_fd = open("status.fifo", O_WRONLY);
    if (status_fd < 0) {
        fprintf(stderr, "Can't open 'status.fifo': %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    key_t key = ftok("test.txt", 0);
    if (key < 0) {
        fprintf(stderr, "Can't find file test.txt in directory");
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }

    int shmid = shmget(key, 1024 * N, IPC_CREAT | SHM_RDONLY | 0666);
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
    while (get_dish(&dish, &memory, &position) != -1) {
        size_t time = -1;
        for (size_t i = 0; i < dishes.size; i++) {
            if (strcmp(dishes.array[i].type, dish.type) == 0)
                time = dishes.array[i].time;
        }

        if (time == -1) {
            fprintf(stderr, "Unknown dish type: '%s'\n", dish.type);
            continue;
        }
        fprintf(stdout, "Start to wiping dish with type: %s. It will be take %zu seconds.\n", dish.type, time);
        sleep(time);
        write(status_fd, "1", 2);
    }

    clean_memory(&table);
    clean_memory(&dishes);
    write(status_fd, "-1", 3);
    close(status_fd);
    shmdt(memory);
    return 0;
}

int get_dish(dish_t *dish, char **memory, size_t *position) {
    size_t size = strlen(*memory + *position);

    if (size == 0) {
        return -1;
    }

    printf("Take from table dish with type: %s'", *memory + *position);

    dish->type = calloc(size + 1, sizeof(char));
    strcpy(dish->type, *memory + *position);
    *position += size + 1;

    return 0;
}
