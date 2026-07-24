CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -Iinclude -g

SRC = src/decoder.c src/formatter.c src/jules_disasm.c
OBJ = $(SRC:.c=.o)
TEST_SRC = tests/test_main.c
TEST_OBJ = $(TEST_SRC:.c=.o)

all: libjules_disasm.a test

libjules_disasm.a: $(OBJ)
	ar rcs $@ $(OBJ)

test: $(TEST_OBJ) libjules_disasm.a
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f src/*.o tests/*.o libjules_disasm.a test

.PHONY: all clean
