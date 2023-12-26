#include <errno.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

size_t MAX_N, N;

enum { MAX_ARGS = 16, MAX_LENGTH = 4096 };

void signal_handler(int signal) {
    int result_code;
    pid_t child_pid;
    while ((child_pid = waitpid(-1, &result_code, WNOHANG)) > 0) {
        N--;
        printf("Process with PID:%d finished", child_pid);
        if (WIFEXITED(result_code)) {
            printf(" normal with return code: %d.", WEXITSTATUS(result_code));
        } else if (WIFSIGNALED(result_code)) {
            printf(" by signal %d.", WTERMSIG(result_code));
        }
        printf(" Now running %ld of %ld\n", N, MAX_N);
    }
}

size_t parse_line(char **program_name, char ***arguments, char *line);
int process_arguments(int args_count, char **args, int *kill_children);

int main(int argc, char **argv) {
    signal(SIGCHLD, signal_handler);
    int kill_children = 0;
    int r = process_arguments(argc, argv, &kill_children);
    if (r != 0) return r;
    N = 0;

    char *program_name = malloc(MAX_LENGTH * sizeof(char));
    char **program_arguments = malloc(MAX_ARGS * sizeof(char *));

    for (int i = 0; i < MAX_ARGS; i++) {
        program_arguments[i] = malloc(MAX_LENGTH * sizeof(char));
        strcpy(program_arguments[i], "");
    }

    size_t line_size = MAX_LENGTH * sizeof(char);
    char *line = malloc(line_size);
    size_t read = 0;

    while ((read = getline(&line, &line_size, stdin)) != -1) {
        if (line[read - 1] == '\n') line[read - 1] = 0;

        size_t result = parse_line(&program_name, &program_arguments, line);

        if (result < 2) {
            printf("Not enough data in line: '%s'\n", line);
            continue;
        }

        if (N == MAX_N) {
            printf("Can't run one more program. Now running %ld of %ld\n", N,
                   MAX_N);
        } else if (N < MAX_N) {
            N++;
            pid_t p = fork();

            if (p > 0) {
                continue;
            } else if (p == 0) {
                program_arguments[result - 1] = NULL;
                if (execvp(program_name, program_arguments) == -1) {
                    printf("Can't run program: %s\n", strerror(errno));
                    return 1;
                }
            }
        }
    }

    printf("Get EOF file. Or something went wrong with stdin. Goodbye!\n");

    free(program_name);
    for (int i = 0; i < MAX_ARGS; i++) {
        free(program_arguments[i]);
    }
    free(program_arguments);

    if (kill_children) {
        kill(0, SIGKILL);
    }
    return 0;
}

int process_arguments(int args_count, char **args, int *kill_children) {
    if (args_count == 2 && (strcmp(args[1], "--help") == 0 || strcmp(args[1], "-h") == 0)) {
        printf("Usage: %s [N]\n", args[0]);
        printf("N - the maximum number of programs that can run in parallel\n");
        printf("Options:\n");
        printf("\t-h, --help\tdisplay this help and exit\n");
        return 0;
    }

    if (args_count <= 3) {
        long result = strtol(args[1], NULL, 10);
        if (result != 0) {
            MAX_N = result;
        } else {
            printf("Can't convert N: %s", strerror(errno));
            return 1;
        }
    }

    if (args_count == 3 && strcmp(args[2], "-kc") == 0) {
        *kill_children = 1;
        return 0;
    } else if (args_count == 3) {
        printf("Unknown argument: %s", args[2]);
        return -1;
    }
    return 0;
}

size_t parse_line(char **program_name, char ***arguments, char *line) {
    size_t index = 0;
    char *token = strtok(line, " ");
    for (index = 0; token != NULL; index++) {
        if (index == 0) {
            strcpy(*program_name, token);
        } else {
            strcpy((*arguments)[index], token);
        }
        token = strtok(NULL, " ");
    }
    strcpy((*arguments)[0], *program_name);
    ++index;
    return index;
}
