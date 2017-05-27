TARGET = program
CC = gcc
C_COMP_FLAG = -lm -Wall -Wextra -Werror -pedantic-errors -DNDEBUG

.PHONY: default all clean

default: $(TARGET)
all: default

OBJECTS = $(patsubst %.c, %.o, $(wildcard *.c))

.PRECIOUS: $(TARGET) $(OBJECTS)

$(TARGET): $(OBJECTS)
	$(CC) $(C_COMP_FLAG) sch.c -o sch

clean:
	rm -f *.o
	rm -f $(TARGET)
