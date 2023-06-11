CC = gcc
CFLAGS = -Wall -Wextra -std=c11

SRCS = main.c structs.c
OBJS = $(SRCS:.c=.o)
TARGET = program

.PHONY: all clean

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) $(OBJS) -o $(TARGET) -pthread

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)