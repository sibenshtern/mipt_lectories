#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <stdbool.h>
#include <fcntl.h>

#include "common.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int get_dish(dish_t *dish, int sockid);

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

    int sockid = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockid < 0) {
        fprintf(stderr, "Can get socket: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_in addr_srv = {};
    addr_srv.sin_family = AF_INET;
    addr_srv.sin_addr.s_addr = htons(INADDR_ANY);
    addr_srv.sin_port = htons(1616);
    int lenaddr_srv = sizeof(addr_srv);

    if (bind(sockid, (struct sockaddr *) &addr_srv, lenaddr_srv) < 0) {
        fprintf(stderr, "Can't bind socket: %s", strerror(errno));
        return -1;
    }

    dish_t dish = {0};
    while (get_dish(&dish, sockid) != -1) {
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
    return 0;
}

int get_dish(dish_t *dish, int sockid) {
    char buffer[1024];
    memset(buffer, 0, 1024);
    struct sockaddr_in addr_cli;
    int lenaddr_cli = sizeof(addr_cli);
    size_t size = recvfrom(sockid, buffer, 1024, 0, (struct sockaddr *) &addr_cli, &lenaddr_cli);

    if (size == -1) return -1;

    if (strcmp(buffer, "-1") == 0) {
        return -1;
    }

    if (!dish->type) {
        dish->type = calloc(size + 1, sizeof(char));
    } else if (strlen(dish->type) + 1 < size) {
        dish->type = realloc(dish->type, size + 1);
    }
    strcpy(dish->type, buffer);

    return 0;
}