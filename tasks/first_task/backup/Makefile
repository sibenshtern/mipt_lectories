CC = gcc
CFLAGS = -Werror -Wextra -Wall
LIBRARIES = -lm -lz

SOURCES = backup.c utils.c main.c log.c
OBJECTS = $(SOURCES:.c=.o)

TARGET = backup

all: backup

.c.o:
	$(CC) $(CFLAGS)  -c $< -o $@ $(LIBRARIES)

clean:
	rm -rf *.o

backup: $(OBJECTS)
	$(CC) $(CFLAGS) -o $(TARGET) $(OBJECTS) $(LIBRARIES)
