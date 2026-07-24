#ifndef ELF_LOADER_H
#define ELF_LOADER_H

#include "emu.h"

// Returns 0 on success, -1 on failure
int load_elf(CPU *cpu, const char *filepath);

#endif // ELF_LOADER_H
