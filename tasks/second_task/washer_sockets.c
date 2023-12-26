#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <fcntl.h>

#include "common.h"

#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int send_dish(dish_t *dish, int sockid, struct sockaddr_in addr_srv);

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

    int sockid = socket(PF_INET, SOCK_DGRAM, 0);
    if (sockid < 0) {
        fprintf(stderr, "Can't create socket: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_in addr_cli = {0};
    addr_cli.sin_family = AF_INET;
    addr_cli.sin_addr.s_addr = htonl(INADDR_ANY);
    addr_cli.sin_port = 0;
    int lenaddr_cli = sizeof(addr_cli);

    if (bind(sockid, (struct sockaddr *)&addr_cli, lenaddr_cli) < 0) {
        fprintf(stderr, "Can't bind: %s\n", strerror(errno));
        return -1;
    }

    struct sockaddr_in addr_srv = {};
    addr_srv.sin_family = AF_INET;
    addr_srv.sin_addr.s_addr = inet_addr("127.0.0.1");
    addr_srv.sin_port = htons(1616);

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
            send_dish(&dish, sockid, addr_srv);
            ++table_size;
        }
    }
    fclose(file);
    sendto(sockid, "-1", 3, 0, (struct sockaddr *)&addr_srv, sizeof(addr_srv));
    char *status = NULL;
    size_t size = 0;
    while (read_pipe(&status, '\0', &size, status_fd) > 0 && strcmp(status, "-1") != 0)
        continue;
    close(status_fd);
    return 0;
}

int send_dish(dish_t *dish, int sockid, struct sockaddr_in addr_srv) {
    if (sendto(sockid, dish->type, strlen(dish->type) + 1, 0, (struct sockaddr *)&addr_srv, sizeof(addr_srv)) < 0) {
        fprintf(stderr, "Can't send dish type to wiper\n");
        return -1;
    }
    return 0;
}