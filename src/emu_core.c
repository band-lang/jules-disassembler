#include "emu.h"
#include "emu_ops.h"
#include "decoder.h"
#include <stdio.h>
#include <string.h>

void cpu_init(CPU *cpu) {
    memset(cpu, 0, sizeof(CPU));
    cpu->rsp = MEMORY_SIZE - 16; // Set stack pointer as requested
}

int cpu_step(CPU *cpu) {
    if (cpu->rip >= MEMORY_SIZE) {
        fprintf(stderr, "Emulator Error: RIP (0x%lx) out of bounds.\n", cpu->rip);
        return -1;
    }

    jd_decoded_inst_t inst;
    jd_status_t status = decode_instruction(&cpu->memory[cpu->rip], MEMORY_SIZE - cpu->rip, &inst);
    if (status != JD_SUCCESS) {
        fprintf(stderr, "Emulator Error: Failed to decode instruction at RIP 0x%lx (status %d)\n", cpu->rip, status);
        return -1;
    }

    uint64_t next_rip = cpu->rip + inst.length;

    // Execute instruction
    int ret = emu_execute(cpu, &inst, next_rip);

    if (ret == 1) {
        // HLT instruction was executed
        cpu->rip = next_rip;
        return 1;
    } else if (ret != 0) {
        fprintf(stderr, "Emulator Error: Execution failed at RIP 0x%lx\n", cpu->rip);
        return -1;
    }

    // Advance RIP if the instruction did not modify it (e.g. JMP/CALL modifies it)
    if (cpu->rip == next_rip - inst.length) {
        cpu->rip = next_rip;
    }

    return 0; // Success
}

int cpu_run(CPU *cpu, uint64_t max_instructions) {
    uint64_t count = 0;
    while (count < max_instructions) {
        int status = cpu_step(cpu);
        if (status == 1) { // HLT
            return 0;
        } else if (status != 0) { // Error
            return -1;
        }
        count++;
    }
    return 0; // Reached max instructions without error
}
