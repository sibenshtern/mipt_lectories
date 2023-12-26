#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/sem.h>

#include "common.h"

size_t N = 0;

int get_dish(dish_t *dish, int semid);

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

    key_t key = ftok("test.txt", 0);
    if (key == -1) {
        fprintf(stderr,
                "Can't generate key for semaphore with file 'test.txt': '%s'\n", strerror(errno));
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }

    int semid = semget(key, 1, IPC_CREAT | 0666);
    if (semid < 0) {
        fprintf(stderr, "Can't get semaphore: '%s'\n", strerror(errno));
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }

    dish_t dish = {0};
    while (get_dish(&dish, semid) != -1) {
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
    }

    clean_memory(&table);
    clean_memory(&dishes);
    return 0;
}

int get_dish(dish_t *dish, int semid) {
    printf("Take dish from table\n");
    struct sembuf plus = {0, +1, 0},
            minus = {0, -1, 0},
            wait = {0, 0, 0},
            minus1 = {1, -1, 0},
            plus1 = {1, +1, 0};

    make_operation(&minus, semid);
    make_operation(&plus1, semid);

    FILE *file = fopen("send.txt", "r+");
    char *line = NULL;
    size_t index = 0;
    size_t capacity = 0;

    size_t size = 0;
    size_t arr_capacity = 10;
    char **lines = calloc(arr_capacity, sizeof(char *));
    for (int i = 0; i < arr_capacity; i++)
        lines[i] = calloc(100, sizeof(char));

    while ((size = getline(&line, &capacity, file)) != -1) {
        if (index == arr_capacity) {
            lines = realloc(lines, arr_capacity * 2 * sizeof(char *));
            arr_capacity *= 2;
            for (int i = arr_capacity / 2; i < arr_capacity; i++)
                lines[i] = calloc(100, sizeof(char));
        }

        if (size > 100)
            lines[index] = realloc(lines[index], size + 1);

        strcpy(lines[index], line);
        index++;
    }

    char *type = calloc(strlen(lines[0]) + 1, sizeof(char));
    strcpy(type, lines[0]);

    size_t len = strlen(type);
    if (type[len - 1] == '\n') type[len - 1] = 0;

    if (!dish->type) {
        dish->type = calloc(len + 1, sizeof(char));
        dish->time = -1;
    } else if (dish->type && strlen(dish->type) < len + 1) {
        dish->type = realloc(dish->type, len + 1);
    }
    strcpy(dish->type, type);

    fclose(file);

    file = fopen("send.txt", "w");
    fclose(file);

    file = fopen("send.txt", "r+");
    for (int i = 1; i < index; i++) {
        fprintf(file, "%s", lines[i]);
    }

    fclose(file);
    make_operation(&plus, semid);
    printf("Take one dish from table with type{'%s'}\n", dish->type);

    return 0;
}
