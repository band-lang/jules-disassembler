#ifndef EMU_H
#define EMU_H

#include <stdint.h>
#include <stdbool.h>

#define MEMORY_SIZE (16 * 1024 * 1024)

// RFLAGS macros
#define FLAG_CF (1 << 0)
#define FLAG_PF (1 << 2)
#define FLAG_AF (1 << 4)
#define FLAG_ZF (1 << 6)
#define FLAG_SF (1 << 7)
#define FLAG_OF (1 << 11)

typedef struct {
    // General-purpose registers
    uint64_t rax, rcx, rdx, rbx, rsp, rbp, rsi, rdi;
    uint64_t r8, r9, r10, r11, r12, r13, r14, r15;

    uint64_t rip;    // Instruction pointer
    uint64_t rflags; // Flags register

    // Memory (16 MB virtual address space)
    uint8_t memory[MEMORY_SIZE];
} CPU;

void cpu_init(CPU *cpu);
int cpu_step(CPU *cpu);
int cpu_run(CPU *cpu, uint64_t max_instructions);

#endif // EMU_H
