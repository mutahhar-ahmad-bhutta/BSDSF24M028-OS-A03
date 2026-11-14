CC = gcc
CFLAGS = -Wall -Iinclude
LDFLAGS =

SRC = $(wildcard src/*.c)
OBJ = $(patsubst src/%.c,obj/%.o,$(SRC))
TARGET = bin/myshell

all: $(TARGET)

$(TARGET): $(OBJ)
	mkdir -p bin
	$(CC) $(OBJ) -o $(TARGET) $(LDFLAGS)

obj/%.o: src/%.c include/shell.h
	mkdir -p obj
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf obj bin
