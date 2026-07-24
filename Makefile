CC = gcc
CFLAGS = -Wall -Wextra -pedantic -std=c11 -Iinclude -Isrc -g

DISASM_SRC = src/decoder.c src/formatter.c src/jules_disasm.c
DISASM_OBJ = $(DISASM_SRC:.c=.o)
TEST_DISASM_SRC = tests/test_main.c
TEST_DISASM_OBJ = $(TEST_DISASM_SRC:.c=.o)

EMU_SRC = src/emu_core.c src/emu_ops.c src/elf_loader.c
EMU_OBJ = $(EMU_SRC:.c=.o)
TEST_EMU_SRC = tests/test_emu.c
TEST_EMU_OBJ = $(TEST_EMU_SRC:.c=.o)

all: libjules_disasm.a test test_emu test_elf

libjules_disasm.a: $(DISASM_OBJ)
	ar rcs $@ $(DISASM_OBJ)

test: $(TEST_DISASM_OBJ) libjules_disasm.a
	$(CC) $(CFLAGS) -o $@ $^

test_emu: $(TEST_EMU_OBJ) $(EMU_OBJ) libjules_disasm.a
	$(CC) $(CFLAGS) -o $@ $^

test_elf: test.c
	$(CC) -static -nostdlib -O0 -fcf-protection=none -o $@ $<

clean:
	rm -f src/*.o tests/*.o libjules_disasm.a test test_emu test_elf

.PHONY: all clean
