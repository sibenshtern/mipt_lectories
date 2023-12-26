#include <sys/mman.h>
#include <sys/types.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdio.h>

int main() {
    char *message = "Hello, World!";
    size_t message_length = strlen(message) * sizeof(char);

    int fd = open("test.txt", O_CREAT | O_RDWR, 0600);
    ftruncate(fd, message_length);

    void *mem = mmap(NULL, message_length, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (mem == MAP_FAILED) {
        perror("mnap");
        return -1;
    }

    // like letters
    // char *text = (char *)mem;

    // strcpy(text, message);


    munmap(mem, message_length);
    close(fd);
    return 0;
}