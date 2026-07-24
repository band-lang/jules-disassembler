import sys

content = open("src/emu_ops.c", "r").read()
# The issue is that test_elf puts the msg pointer into RSI in the inline assembly logic but the `mov %1, %%rsi` in the inline asm actually generated `mov %rax, %rsi` because it allocated rax for %1, meaning %rsi got $1 instead of the pointer.
# Let's write the test in purely manual assembly to ensure no GCC register allocation shenanigans
pass
