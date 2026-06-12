CC      = gcc
CFLAGS  = -Wall -Wextra -std=c99 -g
TARGET  = mini_compiler
SRCS    = mini_compiler.c lexer.c parser.c semantic.c symbol_table.c error.c codegen.c
OBJS    = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS) $(TARGET)

demo: $(TARGET)
	./$(TARGET) --demo

.PHONY: all clean demo
