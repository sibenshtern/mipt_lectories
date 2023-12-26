#include "log.h"

void logging(FILE *file, char letter_mode, char *message) {
    time_t now = time(NULL);

    char *mode = NULL;
    if (letter_mode == 'D')
        mode = "DEBUG";
    else if (letter_mode == 'I')
        mode = "INFO";
    else if (letter_mode == 'W')
        mode = "WARNING";
    else if (letter_mode == 'E')
        mode = "ERROR";

    struct tm *time = localtime(&now);
    char str_time[64];
    strftime(str_time, 64, "%Y-%m-%d %H:%M:%S", time);

    fprintf(file, "%s %s \"%s\"\n", mode, str_time, message);
}
