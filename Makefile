CC = gcc
CFLAGS = -Wall -Wextra -I.

TARGET = liteshell
TEST_TARGET = test_suite

SRC = shell/main.c \
      shell/parser.c \
      shell/executor.c \
      shell/history.c \
      core/command_registry.c \
      commands/filesystem_ops.c \
      commands/system_ops.c \
      commands/task_ops.c \
      commands/monitor_ops.c \
      commands/builtin.c \
      diagnostics/diagnostic.c \
      observation/observation.c \
      ui/terminal.c \
      ui/screen.c

OBJ = $(SRC:.c=.o)

TEST_SRC = tests/test_suite.c \
           shell/parser.c \
           shell/executor.c \
           shell/history.c \
           core/command_registry.c \
           commands/filesystem_ops.c \
           commands/system_ops.c \
           commands/task_ops.c \
           commands/monitor_ops.c \
           commands/builtin.c \
           diagnostics/diagnostic.c \
           observation/observation.c \
           ui/terminal.c \
           ui/screen.c

TEST_OBJ = $(TEST_SRC:.c=.o)

all: $(TARGET) $(TEST_TARGET)

$(TARGET): $(OBJ)
	$(CC) $(OBJ) -o $(TARGET)

$(TEST_TARGET): $(TEST_OBJ)
	$(CC) $(TEST_OBJ) -o $(TEST_TARGET)

test: $(TEST_TARGET)
	./$(TEST_TARGET)

clean:
	rm -f $(OBJ) $(TEST_OBJ) $(TARGET) $(TEST_TARGET)

.PHONY: all test clean
