#ifndef DECODER_H
#define DECODER_H

#include <stdint.h>
#include <stdbool.h>
#include <stddef.h>
#include "jules_disasm.h"

// Registers
typedef enum {
    REG_AL=0, REG_CL, REG_DL, REG_BL, REG_AH, REG_CH, REG_DH, REG_BH, REG_SPL, REG_BPL, REG_SIL, REG_DIL, REG_R8B, REG_R9B, REG_R10B, REG_R11B, REG_R12B, REG_R13B, REG_R14B, REG_R15B,
    REG_AX=20, REG_CX, REG_DX, REG_BX, REG_SP, REG_BP, REG_SI, REG_DI, REG_R8W, REG_R9W, REG_R10W, REG_R11W, REG_R12W, REG_R13W, REG_R14W, REG_R15W,
    REG_EAX=40, REG_ECX, REG_EDX, REG_EBX, REG_ESP, REG_EBP, REG_ESI, REG_EDI, REG_R8D, REG_R9D, REG_R10D, REG_R11D, REG_R12D, REG_R13D, REG_R14D, REG_R15D,
    REG_RAX=60, REG_RCX, REG_RDX, REG_RBX, REG_RSP, REG_RBP, REG_RSI, REG_RDI, REG_R8, REG_R9, REG_R10, REG_R11, REG_R12, REG_R13, REG_R14, REG_R15,
    REG_RIP=80,
    REG_NONE=99
} jd_reg_t;

// Operand types
typedef enum {
    OP_NONE = 0,
    OP_REG,
    OP_MEM,
    OP_IMM,
    OP_REL
} jd_op_type_t;

// Memory operand
typedef struct {
    jd_reg_t base;
    jd_reg_t index;
    uint8_t scale; // 1, 2, 4, 8
    int32_t disp;
    bool has_disp;
    uint8_t disp_size; // 0, 1, 4 bytes
} jd_mem_op_t;

// Operand
typedef struct {
    jd_op_type_t type;
    union {
        jd_reg_t reg;
        jd_mem_op_t mem;
        int64_t imm;
        int64_t rel; // relative offset
    } val;
    uint8_t size; // Size in bytes (1, 2, 4, 8)
} jd_operand_t;

// Mnemonic
typedef enum {
    MNEM_NONE = 0,
    MNEM_MOV, MNEM_ADD, MNEM_SUB, MNEM_AND, MNEM_OR, MNEM_XOR, MNEM_CMP,
    MNEM_INC, MNEM_DEC, MNEM_MUL, MNEM_DIV, MNEM_PUSH, MNEM_POP,
    MNEM_CALL, MNEM_RET, MNEM_JMP, MNEM_JE, MNEM_JNE, MNEM_JL, MNEM_JG,
    MNEM_JLE, MNEM_JGE, MNEM_NOP, MNEM_LEA,
    MNEM_IDIV, MNEM_IMUL, MNEM_NOT, MNEM_NEG, MNEM_TEST,
    MNEM_XCHG, MNEM_SHL, MNEM_SHR, MNEM_SAR, MNEM_LOOP, MNEM_HLT
} jd_mnem_t;

// Internal representation of an instruction
typedef struct {
    jd_mnem_t mnem;
    jd_operand_t op1;
    jd_operand_t op2;
    uint8_t length; // Instruction length in bytes
    bool has_rex;
    uint8_t rex;    // The REX byte value if present
    uint8_t rex_w;
    uint8_t rex_r;
    uint8_t rex_x;
    uint8_t rex_b;
    uint8_t op_size_override; // 0x66 prefix
    uint8_t addr_size_override; // 0x67 prefix
    bool is_rip_relative; // Flag for printing rip+disp
} jd_decoded_inst_t;

jd_status_t decode_instruction(const uint8_t *buffer, size_t buffer_size, jd_decoded_inst_t *inst);

#endif // DECODER_H
