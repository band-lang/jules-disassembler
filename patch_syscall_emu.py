import sys

content = open("src/emu_ops.c", "r").read()
# Add syscall to emu_ops.c
content = content.replace("    switch (inst->mnem) {\n        case MNEM_HLT:\n            return 1;", """    switch (inst->mnem) {
        case MNEM_SYSCALL:
            // Minimal syscall implementation for emulator
            if (cpu->rax == 1) { // sys_write
                uint64_t fd = cpu->rdi;
                uint64_t buf_addr = cpu->rsi;
                uint64_t count = cpu->rdx;
                if (fd == 1) { // stdout
                    for(uint64_t i = 0; i < count; i++) {
                        putchar(cpu->memory[buf_addr + i]);
                    }
                }
                cpu->rax = count;
            } else if (cpu->rax == 60) { // sys_exit
                return 1; // exit loop
            } else {
                fprintf(stderr, "Emulator Error: Unsupported syscall %lu\\n", cpu->rax);
                return -1;
            }
            break;
        case MNEM_HLT:
            return 1;""")

with open("src/emu_ops.c", "w") as f:
    f.write(content)
