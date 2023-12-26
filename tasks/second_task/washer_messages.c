#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <fcntl.h>

#include "common.h"

#include <sys/msg.h>

int send_dish(dish_t *dish, int msgid);

typedef struct msgbuf {
    long mtype;
    char mtext[1024];
} message_t;

size_t MAX_TABLE_SIZE = 0;

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
    MAX_TABLE_SIZE = strtol(table_limit, &end, 10);
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

    key_t key = ftok("test.txt", 0);
    if (key < 0) {
        fprintf(stderr, "Can't open file test.txt: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    int msgid = msgget(key, IPC_CREAT | 0666);
    if (msgid < 0) {
        fprintf(stderr, "Can't get message queue id: %s\n", strerror(errno));
        return EXIT_FAILURE;
    }

    printf("%d\n", MAX_TABLE_SIZE);
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
            if (table_size >= MAX_TABLE_SIZE) {
                message_t m;
                if (msgrcv(msgid, &m, sizeof(m) - sizeof(m.mtype), 2, 0) >= 0)
                    --table_size;
            }

            send_dish(&dish, msgid);
            ++table_size;
        }
    }
    fclose(file);

    message_t message;
    message.mtype = 1;
    strcpy(message.mtext, "-1");
    msgsnd(msgid, &message, sizeof(message) - sizeof(message.mtype), 0);

    return 0;
}

int send_dish(dish_t *dish, int msgid) {
    message_t message;
    message.mtype = 1;
    size_t type_size = strlen(dish->type) + 1;
    for (size_t i = 0; i < type_size; i++) {
        message.mtext[i] = dish->type[i];
    }

    if (msgsnd(msgid, &message, sizeof(message_t) - sizeof(message.mtype), 0) < 0) {
        fprintf(stderr, "Can't send message: %s\n", strerror(errno));
        return -1;
    }
    return 0;
}