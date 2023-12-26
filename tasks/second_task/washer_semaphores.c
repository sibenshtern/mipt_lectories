#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include "common.h"

#include <sys/sem.h>

size_t N = 0;

int send_dish(dish_t *dish, int semid);
int load_dish(dish_t *dish, FILE *file);

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

    key_t key = ftok("test.txt", 0);
    if (key == -1) {
        fprintf(stderr,
                "Can't generate key for semaphore with file 'test.txt': '%s'\n", strerror(errno));
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }

    int semid = semget(key, 2, IPC_CREAT | 0666);
    if (semid < 0) {
        fprintf(stderr, "Can't get semaphore: '%s'\n", strerror(errno));
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }
    semctl(semid, 0, SETVAL, 1);
    semctl(semid, 1, SETVAL, N);

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
            send_dish(&dish, semid);
        }
    }
    fclose(file);
    return 0;
}

int send_dish(dish_t *dish, int semid) {
    printf("Washed dish with type: %s\n", dish->type);
    struct sembuf plus = {0, +1, 0},
            minus = {0, -1, 0},
            minus1 = {1, -1, 0};

    make_operation(&minus1, semid);
    make_operation(&minus, semid);

    FILE *file = fopen("send.txt", "a");
    fprintf(file, "%s\n", dish->type);
    fclose(file);

    make_operation(&plus, semid);
    printf("Place dish to table\n");
    return 0;
}
