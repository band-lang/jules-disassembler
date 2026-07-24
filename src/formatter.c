#include "formatter.h"
#include <stdio.h>
#include <string.h>
#include <inttypes.h>

static const char* reg_names[] = {
    // 8-bit (0-19)
    "al", "cl", "dl", "bl", "ah", "ch", "dh", "bh", "spl", "bpl", "sil", "dil", "r8b", "r9b", "r10b", "r11b", "r12b", "r13b", "r14b", "r15b",
    // 16-bit (20-39)
    "ax", "cx", "dx", "bx", "sp", "bp", "si", "di", "r8w", "r9w", "r10w", "r11w", "r12w", "r13w", "r14w", "r15w", "", "", "", "",
    // 32-bit (40-59)
    "eax", "ecx", "edx", "ebx", "esp", "ebp", "esi", "edi", "r8d", "r9d", "r10d", "r11d", "r12d", "r13d", "r14d", "r15d", "", "", "", "",
    // 64-bit (60-79)
    "rax", "rcx", "rdx", "rbx", "rsp", "rbp", "rsi", "rdi", "r8", "r9", "r10", "r11", "r12", "r13", "r14", "r15", "", "", "", "",
    // RIP (80)
    "rip"
};

static const char* mnem_names[] = {
    "none",
    "mov", "add", "sub", "and", "or", "xor", "cmp",
    "inc", "dec", "mul", "div", "push", "pop",
    "call", "ret", "jmp", "je", "jne", "jl", "jg",
    "jle", "jge", "nop", "lea",
    "idiv", "imul", "not", "neg", "test"
};

static void format_operand(const jd_operand_t *op, uint64_t address, uint8_t inst_len, char *out, size_t size, bool force_size) {
    if (op->type == OP_REG) {
        snprintf(out, size, "%s", reg_names[op->val.reg]);
    } else if (op->type == OP_IMM) {
        if (op->val.imm < 0 && op->size == 1) { // maybe format nice negs, but let's just do hex
            snprintf(out, size, "0x%" PRIx64, (uint64_t)(op->val.imm & 0xFF));
        } else if (op->val.imm < 0 && op->size == 2) {
            snprintf(out, size, "0x%" PRIx64, (uint64_t)(op->val.imm & 0xFFFF));
        } else if (op->val.imm < 0 && op->size == 4) {
            snprintf(out, size, "0x%" PRIx64, (uint64_t)(op->val.imm & 0xFFFFFFFF));
        } else {
            snprintf(out, size, "0x%" PRIx64, (uint64_t)op->val.imm);
        }
    } else if (op->type == OP_REL) {
        uint64_t target = address + inst_len + op->val.rel;
        snprintf(out, size, "0x%" PRIx64, target);
    } else if (op->type == OP_MEM) {
        const char *size_str = "";
        if (force_size) {
            if (op->size == 1) size_str = "byte ptr ";
            else if (op->size == 2) size_str = "word ptr ";
            else if (op->size == 4) size_str = "dword ptr ";
            else if (op->size == 8) size_str = "qword ptr ";
        }

        char base_str[32] = {0};
        char idx_str[32] = {0};
        char disp_str[32] = {0};

        if (op->val.mem.base != REG_NONE) {
            snprintf(base_str, sizeof(base_str), "%s", reg_names[op->val.mem.base]);
        }

        if (op->val.mem.index != REG_NONE) {
            if (op->val.mem.scale > 1) {
                snprintf(idx_str, sizeof(idx_str), "%s*%d", reg_names[op->val.mem.index], op->val.mem.scale);
            } else {
                snprintf(idx_str, sizeof(idx_str), "%s", reg_names[op->val.mem.index]);
            }
        }

        if (op->val.mem.has_disp) {
            if (op->val.mem.base == REG_NONE && op->val.mem.index == REG_NONE) {
                // Absolute addressing (disp32)
                snprintf(disp_str, sizeof(disp_str), "0x%" PRIx32, (uint32_t)op->val.mem.disp); // print as unsigned
            } else {
                if (op->val.mem.disp < 0) {
                    snprintf(disp_str, sizeof(disp_str), "-0x%" PRIx32, (uint32_t)(-op->val.mem.disp));
                } else if (op->val.mem.disp > 0 || (op->val.mem.disp == 0 && op->val.mem.disp_size > 0)) {
                    // Technically disp0 shouldn't have disp_str unless it's explicitly disp8/32 = 0
                    if (op->val.mem.disp > 0)
                        snprintf(disp_str, sizeof(disp_str), "+0x%" PRIx32, (uint32_t)op->val.mem.disp);
                }
            }
        }

        if (op->val.mem.base == REG_RIP) {
            snprintf(out, size, "%s[rip%s]", size_str, disp_str);
        } else if (base_str[0] && idx_str[0]) {
            snprintf(out, size, "%s[%s+%s%s]", size_str, base_str, idx_str, disp_str);
        } else if (base_str[0]) {
            snprintf(out, size, "%s[%s%s]", size_str, base_str, disp_str);
        } else if (idx_str[0]) {
            snprintf(out, size, "%s[%s%s]", size_str, idx_str, disp_str);
        } else {
            snprintf(out, size, "%s[%s]", size_str, disp_str); // absolute
        }
    }
}

void format_instruction(const jd_decoded_inst_t *inst, uint64_t address, char *out_str, size_t out_size) {
    if (inst->mnem == MNEM_NONE) {
        snprintf(out_str, out_size, "invalid");
        return;
    }

    char op1_str[64] = {0};
    char op2_str[64] = {0};

    bool force_size = false;
    if (inst->op1.type == OP_MEM) force_size = true;
    if (inst->op2.type == OP_NONE && inst->op1.type == OP_MEM) force_size = true; // e.g. inc [rax]

    if (inst->op1.type != OP_NONE) {
        format_operand(&inst->op1, address, inst->length, op1_str, sizeof(op1_str), force_size);
    }
    if (inst->op2.type != OP_NONE) {
        format_operand(&inst->op2, address, inst->length, op2_str, sizeof(op2_str), force_size);
    }

    if (inst->op1.type != OP_NONE && inst->op2.type != OP_NONE) {
        snprintf(out_str, out_size, "%s %s, %s", mnem_names[inst->mnem], op1_str, op2_str);
    } else if (inst->op1.type != OP_NONE) {
        snprintf(out_str, out_size, "%s %s", mnem_names[inst->mnem], op1_str);
    } else {
        snprintf(out_str, out_size, "%s", mnem_names[inst->mnem]);
    }
}
