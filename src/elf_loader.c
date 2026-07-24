#include "elf_loader.h"
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#define EI_NIDENT 16

typedef struct {
    unsigned char e_ident[EI_NIDENT];
    uint16_t      e_type;
    uint16_t      e_machine;
    uint32_t      e_version;
    uint64_t      e_entry;
    uint64_t      e_phoff;
    uint64_t      e_shoff;
    uint32_t      e_flags;
    uint16_t      e_ehsize;
    uint16_t      e_phentsize;
    uint16_t      e_phnum;
    uint16_t      e_shentsize;
    uint16_t      e_shnum;
    uint16_t      e_shstrndx;
} Elf64_Ehdr;

typedef struct {
    uint32_t p_type;
    uint32_t p_flags;
    uint64_t p_offset;
    uint64_t p_vaddr;
    uint64_t p_paddr;
    uint64_t p_filesz;
    uint64_t p_memsz;
    uint64_t p_align;
} Elf64_Phdr;

#define PT_LOAD 1

int load_elf(CPU *cpu, const char *filepath) {
    FILE *f = fopen(filepath, "rb");
    if (!f) {
        fprintf(stderr, "Emulator Error: Cannot open ELF file '%s'\n", filepath);
        return -1;
    }

    Elf64_Ehdr ehdr;
    if (fread(&ehdr, 1, sizeof(Elf64_Ehdr), f) != sizeof(Elf64_Ehdr)) {
        fprintf(stderr, "Emulator Error: Failed to read ELF header\n");
        fclose(f);
        return -1;
    }

    // Verify magic
    if (memcmp(ehdr.e_ident, "\x7F""ELF", 4) != 0) {
        fprintf(stderr, "Emulator Error: Not an ELF file\n");
        fclose(f);
        return -1;
    }

    // Verify 64-bit
    if (ehdr.e_ident[4] != 2) {
        fprintf(stderr, "Emulator Error: Not a 64-bit ELF\n");
        fclose(f);
        return -1;
    }

    // Read program headers
    fseek(f, ehdr.e_phoff, SEEK_SET);
    Elf64_Phdr *phdrs = malloc(ehdr.e_phnum * sizeof(Elf64_Phdr));
    if (!phdrs) {
        fclose(f);
        return -1;
    }

    if (fread(phdrs, sizeof(Elf64_Phdr), ehdr.e_phnum, f) != ehdr.e_phnum) {
        fprintf(stderr, "Emulator Error: Failed to read program headers\n");
        free(phdrs);
        fclose(f);
        return -1;
    }

    for (int i = 0; i < ehdr.e_phnum; i++) {
        if (phdrs[i].p_type == PT_LOAD) {
            uint64_t vaddr = phdrs[i].p_vaddr;
            uint64_t memsz = phdrs[i].p_memsz;
            uint64_t filesz = phdrs[i].p_filesz;
            uint64_t offset = phdrs[i].p_offset;

            if (vaddr + memsz > MEMORY_SIZE) {
                fprintf(stderr, "Emulator Error: ELF segment out of 16MB bounds (vaddr 0x%lx, memsz 0x%lx)\n", vaddr, memsz);
                free(phdrs);
                fclose(f);
                return -1;
            }

            fseek(f, offset, SEEK_SET);
            if (fread(&cpu->memory[vaddr], 1, filesz, f) != filesz) {
                fprintf(stderr, "Emulator Error: Failed to read segment data\n");
                free(phdrs);
                fclose(f);
                return -1;
            }

            // Zero out bss (memsz > filesz)
            if (memsz > filesz) {
                memset(&cpu->memory[vaddr + filesz], 0, memsz - filesz);
            }
        }
    }

    cpu->rip = ehdr.e_entry;

    free(phdrs);
    fclose(f);
    return 0;
}
