#include <unistd.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

enum {
    MAX_ARGS = 16,
    MAX_PATH = 256,
    MAX_LINE = 4096
};

enum {
    OK = 0,
    ERR_ARGS = 1,
    ERR_OPEN = 2
};

size_t parse_line(int *time, char **program_name, char ***arguments, char *line);

int main(int argc, char **argv) {
    if (argc != 2) {
        printf("Wrong count of arguments. Using: %s [filename]", argv[0]);
        return ERR_ARGS;
    }

    FILE *file = fopen(argv[1], "r");
    if (file == NULL) {
        printf("Can't open file: '%s'", argv[1]);
        return ERR_OPEN;
    }

    int time;
    char *program_name = malloc(MAX_PATH * sizeof(char));
    char **program_arguments = malloc(MAX_ARGS * sizeof(char *));

    for (int i = 0; i < MAX_ARGS; i++) {
        program_arguments[i] = malloc(MAX_PATH * sizeof(char));
        strcpy(program_arguments[i], "");
    }

    size_t line_size = MAX_LINE * sizeof(char);
    char *line = malloc(line_size);
    size_t read;

    while ((read = getline(&line, &line_size, file)) != -1) {
        if (line[read - 1] == '\n')
            line[read - 1] = 0;

        size_t parsed_count = parse_line(&time, &program_name, &program_arguments, line);

        // result >= 3 only when line contains time program_name and arguments
        if (parsed_count < 3) {
            printf("Can't parse line: '%s'\n", line);
            continue;
        }
        
        pid_t pid = fork();
        if (pid > 0) {
            continue;
        } else if (pid == 0) {
            sleep(time);
            program_arguments[parsed_count - 2] = NULL;
            execvp(program_name, program_arguments);
        } 
    }

    // clean all allocated memory
    free(line);
    free(program_name);
    for (int i = 0; i < MAX_ARGS; ++i) {
        free(program_arguments[i]);
    }
    free(program_arguments);

    fclose(file);
    return OK;
}

size_t parse_line(int *time, char **program_name, char ***arguments, char *line) {
    size_t index = 0;
    char *token = strtok(line, " ");
    for (index = 0; token != NULL; index++) {
        if (index == 0) {
            *time = atoi(token);
        } else if (index == 1) {
            strcpy(*program_name, token);
        } else {
            strcpy((*arguments)[index - 1], token);
        }
        token = strtok(NULL, " ");
    }
    strcpy((*arguments)[0], *program_name);
    ++index;
    return index;
}
