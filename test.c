void _start() {
    __asm__ volatile (
        "lea 0x27(%%rip), %%rsi\n\t"
        "mov $1, %%rax\n\t"
        "mov $1, %%rdi\n\t"
        "mov $15, %%rdx\n\t"
        "syscall\n\t"
        "mov $60, %%rax\n\t"
        "mov $0, %%rdi\n\t"
        "syscall\n\t"
        ".ascii \"Hello from VM!\\n\"\n\t"
        ::: "rax", "rdi", "rsi", "rdx", "rcx", "r11", "memory"
    );
}
