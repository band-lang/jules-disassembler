#ifndef JULES_DISASM_H
#define JULES_DISASM_H

#include <stddef.h>
#include <stdint.h>
#include <stdbool.h>

#ifdef __cplusplus
extern "C" {
#endif

// Status codes
typedef enum {
    JD_SUCCESS = 0,
    JD_ERR_NULL_POINTER = -1,
    JD_ERR_INCOMPLETE_INSTRUCTION = -2,
    JD_ERR_INVALID_INSTRUCTION = -3,
    JD_ERR_BUFFER_TOO_SMALL = -4
} jd_status_t;

// Context/Information about a disassembled instruction
typedef struct {
    uint8_t length; // Length of the instruction in bytes
    char formatted_string[128]; // Formatted assembly string
    uint64_t address; // Address of the instruction (useful for relative jumps)
} jd_instruction_t;

/**
 * Disassemble a single x86-64 instruction.
 *
 * @param buffer Pointer to raw bytes of the instruction.
 * @param buffer_size Maximum number of bytes available in the buffer.
 * @param address The logical address of the instruction (used for RIP-relative addressing and relative jumps).
 * @param out_instr Pointer to a jd_instruction_t struct to store the result.
 * @return JD_SUCCESS on success, or an error code on failure.
 */
jd_status_t jd_disassemble(const uint8_t *buffer, size_t buffer_size, uint64_t address, jd_instruction_t *out_instr);

#ifdef __cplusplus
}
#endif

#endif // JULES_DISASM_H
