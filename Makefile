CC = gcc

CFLAGS = -Wall -Wextra -I.

TARGET = liteshell

SRC = shell/main.c \
      shell/parser.c \
      shell/executor.c \
      commands/builtin.c \
      diagnostics/diagnostic.c

OBJ = $(SRC:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

clean:
	rm -f $(OBJ) $(TARGET)

.PHONY: all clean
