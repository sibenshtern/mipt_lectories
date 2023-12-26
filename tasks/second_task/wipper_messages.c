#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>

#include "common.h"

#include <sys/msg.h>
int get_dish(dish_t *dish, int msqid);

typedef struct msgbuf {
    long mtype;
    char mtext[1024];
} message_t;

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

    key_t key = ftok("test.txt", 0);
    if (key < 0) {
        fprintf(stderr, "Can't find file test.txt in directory");
        clean_memory(&dishes);
        return EXIT_FAILURE;
    }

    int msqid = msgget(key, IPC_CREAT | 0666);
    if (msqid < 0) {
        fprintf(stderr, "Can't get message queue id: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }
    dish_t dish = {0};
    while (get_dish(&dish, msqid) != -1) {
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

        message_t message;
        message.mtype = 2;
        msgsnd(msqid, &message, sizeof(message) - sizeof(message.mtype), 0);
    }
    clean_memory(&table);
    clean_memory(&dishes);

    return 0;
}

int get_dish(dish_t *dish, int msqid) {
    message_t message;

    if (msgrcv(msqid, &message, sizeof(message_t) - sizeof(message.mtype), 1, 0) < 0) {
        fprintf(stderr, "Can't receive message: %s\n", strerror(errno));
        return -1;
    }

    if (strcmp(message.mtext, "-1") == 0) return -1;

    size_t type_size = strlen(message.mtext);

    if (!dish->type) {
        dish->type = calloc(type_size + 1, sizeof(char));
    } else if (strlen(dish->type) + 1 < type_size) {
        dish->type = realloc(dish->type, type_size + 1);
    }
    strcpy(dish->type, message.mtext);
    return 0;
}
