#include "decoder.h"
#include "jules_disasm.h"
#include <string.h>

static jd_reg_t get_reg_8bit(uint8_t reg_idx, bool has_rex) {
    if (has_rex) {
        if (reg_idx < 4) return (jd_reg_t)(REG_AL + reg_idx);
        if (reg_idx >= 4 && reg_idx < 8) return (jd_reg_t)(REG_SPL + (reg_idx - 4));
        if (reg_idx >= 8) return (jd_reg_t)(REG_R8B + (reg_idx - 8));
        return REG_NONE;
    } else {
        if (reg_idx < 4) return (jd_reg_t)(REG_AL + reg_idx);
        if (reg_idx >= 4 && reg_idx < 8) return (jd_reg_t)(REG_AH + (reg_idx - 4));
        return REG_NONE;
    }
}

static jd_reg_t get_reg(uint8_t reg_idx, uint8_t size, bool has_rex) {
    if (size == 1) return get_reg_8bit(reg_idx, has_rex);
    if (size == 2) return (jd_reg_t)(REG_AX + reg_idx);
    if (size == 4) return (jd_reg_t)(REG_EAX + reg_idx);
    if (size == 8) return (jd_reg_t)(REG_RAX + reg_idx);
    return REG_NONE;
}

#define MODRM_MOD(b) (((b) >> 6) & 3)
#define MODRM_REG(b) (((b) >> 3) & 7)
#define MODRM_RM(b) ((b) & 7)

#define SIB_SCALE(b) (((b) >> 6) & 3)
#define SIB_INDEX(b) (((b) >> 3) & 7)
#define SIB_BASE(b) ((b) & 7)

static bool decode_modrm(const uint8_t *buffer, size_t buffer_size, size_t *offset, jd_decoded_inst_t *inst, uint8_t size, uint8_t addr_size, jd_operand_t *op_reg, jd_operand_t *op_rm) {
    if (*offset >= buffer_size) return false;
    uint8_t modrm = buffer[(*offset)++];

    uint8_t mod = MODRM_MOD(modrm);
    uint8_t reg = MODRM_REG(modrm) | (inst->rex_r << 3);
    uint8_t rm = MODRM_RM(modrm) | (inst->rex_b << 3);

    if (op_reg) {
        op_reg->type = OP_REG;
        op_reg->val.reg = get_reg(reg, size, inst->has_rex);
        op_reg->size = size;
    }

    if (!op_rm) return true;

    op_rm->size = size;
    if (mod == 3) {
        op_rm->type = OP_REG;
        op_rm->val.reg = get_reg(rm, size, inst->has_rex);
    } else {
        op_rm->type = OP_MEM;
        op_rm->val.mem.scale = 1;
        op_rm->val.mem.index = REG_NONE;
        op_rm->val.mem.has_disp = false;
        op_rm->val.mem.disp = 0;
        op_rm->val.mem.disp_size = 0;

        if ((rm & 7) == 4) { // SIB
            if (*offset >= buffer_size) return false;
            uint8_t sib = buffer[(*offset)++];
            uint8_t scale = SIB_SCALE(sib);
            uint8_t index = SIB_INDEX(sib) | (inst->rex_x << 3);
            uint8_t base = SIB_BASE(sib) | (inst->rex_b << 3);

            op_rm->val.mem.scale = 1 << scale;
            if (index == 4) op_rm->val.mem.index = REG_NONE;
            else op_rm->val.mem.index = get_reg(index, addr_size, inst->has_rex);

            if (mod == 0 && (base & 7) == 5) {
                op_rm->val.mem.base = REG_NONE;
                op_rm->val.mem.has_disp = true;
                op_rm->val.mem.disp_size = 4;
                if (*offset + 4 > buffer_size) return false;
                memcpy(&op_rm->val.mem.disp, buffer + *offset, 4);
                *offset += 4;
            } else {
                op_rm->val.mem.base = get_reg(base, addr_size, inst->has_rex);
            }
        } else {
            if (mod == 0 && (rm & 7) == 5) {
                op_rm->val.mem.base = REG_RIP;
                inst->is_rip_relative = true;
                op_rm->val.mem.has_disp = true;
                op_rm->val.mem.disp_size = 4;
                if (*offset + 4 > buffer_size) return false;
                memcpy(&op_rm->val.mem.disp, buffer + *offset, 4);
                *offset += 4;
            } else {
                op_rm->val.mem.base = get_reg(rm, addr_size, inst->has_rex);
            }
        }

        if (mod == 1) {
            op_rm->val.mem.has_disp = true;
            op_rm->val.mem.disp_size = 1;
            if (*offset + 1 > buffer_size) return false;
            op_rm->val.mem.disp = (int8_t)buffer[(*offset)++];
        } else if (mod == 2) {
            op_rm->val.mem.has_disp = true;
            op_rm->val.mem.disp_size = 4;
            if (*offset + 4 > buffer_size) return false;
            memcpy(&op_rm->val.mem.disp, buffer + *offset, 4);
            *offset += 4;
        }
    }
    return true;
}

jd_status_t decode_instruction(const uint8_t *buffer, size_t buffer_size, jd_decoded_inst_t *inst) {
    if (buffer == NULL || inst == NULL) return JD_ERR_NULL_POINTER;
    if (buffer_size == 0) return JD_ERR_INCOMPLETE_INSTRUCTION;

    memset(inst, 0, sizeof(jd_decoded_inst_t));
    inst->op1.type = OP_NONE;
    inst->op2.type = OP_NONE;

    size_t offset = 0;

    while (offset < buffer_size) {
        uint8_t b = buffer[offset];
        if (b == 0x66) { inst->op_size_override = 1; offset++; }
        else if (b == 0x67) { inst->addr_size_override = 1; offset++; }
        else if (b == 0xF3 || b == 0xF2 || b == 0x2E || b == 0x36 || b == 0x3E || b == 0x26 || b == 0x64 || b == 0x65 || b == 0xF0) { offset++; }
        else break;
    }
    if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;

    uint8_t b = buffer[offset];
    if ((b & 0xF0) == 0x40) {
        inst->has_rex = true; inst->rex = b;
        inst->rex_w = (b >> 3) & 1; inst->rex_r = (b >> 2) & 1; inst->rex_x = (b >> 1) & 1; inst->rex_b = b & 1;
        offset++;
    }
    if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;

    uint8_t op_size = inst->rex_w ? 8 : (inst->op_size_override ? 2 : 4);
    uint8_t addr_size = inst->addr_size_override ? 4 : 8;

    uint8_t opcode = buffer[offset];

    if (opcode == 0xF3) {
        if (offset + 2 < buffer_size && buffer[offset] == 0x0F && buffer[offset+1] == 0x1E && buffer[offset+2] == 0xFA) {
            inst->mnem = MNEM_NOP; // ENDBR64
            offset += 3;
            inst->length = offset;
            return JD_SUCCESS;
        }
    }
    if (opcode == 0xF4) {
        inst->mnem = MNEM_HLT; offset++;
    } else if (opcode == 0x90) {
        inst->mnem = MNEM_NOP; offset++;
    } else if (opcode == 0xC3) {
        inst->mnem = MNEM_RET; offset++;
    } else if (opcode >= 0xB8 && opcode <= 0xBF) { // MOV r32/64, imm32/64
        inst->mnem = MNEM_MOV; offset++;
        inst->op1.type = OP_REG;
        uint8_t reg_idx = (opcode & 7) | (inst->rex_b << 3);
        inst->op1.val.reg = get_reg(reg_idx, op_size, inst->has_rex);
        inst->op1.size = op_size;

        inst->op2.type = OP_IMM; inst->op2.size = op_size;
        if (offset + op_size > buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        uint64_t imm = 0; memcpy(&imm, buffer + offset, op_size);
        inst->op2.val.imm = imm; offset += op_size;
    } else if (opcode >= 0xB0 && opcode <= 0xB7) { // MOV r8, imm8
        inst->mnem = MNEM_MOV; offset++;
        inst->op1.type = OP_REG;
        uint8_t reg_idx = (opcode & 7) | (inst->rex_b << 3);
        inst->op1.val.reg = get_reg(reg_idx, 1, inst->has_rex);
        inst->op1.size = 1;

        inst->op2.type = OP_IMM; inst->op2.size = 1;
        if (offset + 1 > buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        inst->op2.val.imm = buffer[offset++];
    } else if (opcode >= 0x50 && opcode <= 0x57) {
        inst->mnem = MNEM_PUSH; offset++;
        inst->op1.type = OP_REG;
        uint8_t reg_idx = (opcode & 7) | (inst->rex_b << 3);
        inst->op1.val.reg = get_reg(reg_idx, 8, inst->has_rex);
        inst->op1.size = 8;
    } else if (opcode >= 0x58 && opcode <= 0x5F) {
        inst->mnem = MNEM_POP; offset++;
        inst->op1.type = OP_REG;
        uint8_t reg_idx = (opcode & 7) | (inst->rex_b << 3);
        inst->op1.val.reg = get_reg(reg_idx, 8, inst->has_rex);
        inst->op1.size = 8;
    } else if (opcode >= 0x88 && opcode <= 0x8B) {
        inst->mnem = MNEM_MOV; offset++;
        uint8_t size = (opcode & 1) ? op_size : 1;
        bool dir = (opcode & 2) >> 1;
        jd_operand_t *op_dst = dir ? &inst->op1 : &inst->op2;
        jd_operand_t *op_src = dir ? &inst->op2 : &inst->op1;
        if (!decode_modrm(buffer, buffer_size, &offset, inst, size, addr_size, op_dst, op_src)) return JD_ERR_INCOMPLETE_INSTRUCTION;
    } else if (opcode == 0x86 || opcode == 0x87) { // XCHG r/m, r
        uint8_t size = (opcode == 0x87) ? op_size : 1;
        inst->mnem = MNEM_XCHG; offset++;
        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        uint8_t modrm = buffer[offset];

        inst->op1.type = OP_REG;
        inst->op1.val.reg = get_reg(MODRM_REG(modrm), size, inst->rex_r);
        inst->op1.size = size;

        if (!decode_modrm(buffer, buffer_size, &offset, inst, size, addr_size, &inst->op1, &inst->op2)) return JD_ERR_INCOMPLETE_INSTRUCTION;
    } else if (opcode == 0x8D) { // LEA
        inst->mnem = MNEM_LEA; offset++;
        if (!decode_modrm(buffer, buffer_size, &offset, inst, op_size, addr_size, &inst->op1, &inst->op2)) return JD_ERR_INCOMPLETE_INSTRUCTION;
    } else if (opcode == 0xE8) { // CALL rel32
        inst->mnem = MNEM_CALL; offset++;
        inst->op1.type = OP_REL; inst->op1.size = 4;
        if (offset + 4 > buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        int32_t rel = 0; memcpy(&rel, buffer + offset, 4);
        inst->op1.val.rel = rel; offset += 4;
    } else if (opcode == 0xE9) { // JMP rel32
        inst->mnem = MNEM_JMP; offset++;
        inst->op1.type = OP_REL; inst->op1.size = 4;
        if (offset + 4 > buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        int32_t rel = 0; memcpy(&rel, buffer + offset, 4);
        inst->op1.val.rel = rel; offset += 4;
    } else if (opcode == 0xE2) { // LOOP rel8
        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        inst->mnem = MNEM_LOOP; offset++;
        inst->op1.type = OP_REL;
        inst->op1.val.rel = (int8_t)buffer[offset];
        inst->op1.size = 1;
        offset++;
    } else if (opcode == 0xEB) { // JMP rel8
        inst->mnem = MNEM_JMP; offset++;
        inst->op1.type = OP_REL; inst->op1.size = 1;
        if (offset + 1 > buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        inst->op1.val.rel = (int8_t)buffer[offset++];
    } else if (opcode == 0x0F) { // 2-byte opcodes
        offset++;
        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        uint8_t op2 = buffer[offset++];
        if (op2 >= 0x80 && op2 <= 0x8F) {
            jd_mnem_t jcc_map[] = { MNEM_NONE, MNEM_NONE, MNEM_NONE, MNEM_NONE, MNEM_JE, MNEM_JNE, MNEM_NONE, MNEM_NONE, MNEM_NONE, MNEM_NONE, MNEM_NONE, MNEM_NONE, MNEM_JL, MNEM_JGE, MNEM_JLE, MNEM_JG };
            inst->mnem = jcc_map[op2 - 0x80];
            inst->op1.type = OP_REL; inst->op1.size = 4;
            if (offset + 4 > buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
            int32_t rel = 0; memcpy(&rel, buffer + offset, 4);
            inst->op1.val.rel = rel; offset += 4;
        } else if (op2 == 0xAF) { // IMUL
            inst->mnem = MNEM_IMUL;
            if (!decode_modrm(buffer, buffer_size, &offset, inst, op_size, addr_size, &inst->op1, &inst->op2)) return JD_ERR_INCOMPLETE_INSTRUCTION;
        } else {
            return JD_ERR_INVALID_INSTRUCTION;
        }
    } else if ((opcode & 0xC6) == 0) { // ADD, OR, ADC, SBB, AND, SUB, XOR, CMP (ModRM)
        jd_mnem_t alu_map[] = { MNEM_ADD, MNEM_OR, MNEM_NONE, MNEM_NONE, MNEM_AND, MNEM_SUB, MNEM_XOR, MNEM_CMP };
        uint8_t op_group = (opcode >> 3) & 7;
        uint8_t dir = (opcode >> 1) & 1;
        uint8_t size = (opcode & 1) ? op_size : 1;

        if (op_group == 2 || op_group == 3) return JD_ERR_INVALID_INSTRUCTION;

        inst->mnem = alu_map[op_group]; offset++;
        jd_operand_t *op_dst = dir ? &inst->op1 : &inst->op2;
        jd_operand_t *op_src = dir ? &inst->op2 : &inst->op1;
        if (!decode_modrm(buffer, buffer_size, &offset, inst, size, addr_size, op_dst, op_src)) return JD_ERR_INCOMPLETE_INSTRUCTION;
    } else if ((opcode & 0xF8) == 0x40) { // INC / DEC r16/r32/r64 (Wait, in 64-bit mode 40-4F is REX. INC/DEC is FF / FE.
        // Wait, 40-4F is INC/DEC in 32-bit, but in 64-bit it's REX. Handled above.
        // What about FF / FE for INC/DEC?
        return JD_ERR_INVALID_INSTRUCTION;
    } else if (opcode == 0xFF || opcode == 0xFE) { // Grp4 / Grp5
        offset++;
        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        uint8_t modrm = buffer[offset]; // Don't advance offset yet, let decode_modrm do it
        uint8_t reg = MODRM_REG(modrm);
        uint8_t size = (opcode == 0xFE) ? 1 : op_size;

        if (opcode == 0xFE) {
            if (reg == 0) inst->mnem = MNEM_INC;
            else if (reg == 1) inst->mnem = MNEM_DEC;
            else return JD_ERR_INVALID_INSTRUCTION;
        } else {
            if (reg == 0) inst->mnem = MNEM_INC;
            else if (reg == 1) inst->mnem = MNEM_DEC;
            else if (reg == 2) inst->mnem = MNEM_CALL; // CALL r/m
            else if (reg == 4) inst->mnem = MNEM_JMP; // JMP r/m
            else if (reg == 6) inst->mnem = MNEM_PUSH; // PUSH r/m
            else return JD_ERR_INVALID_INSTRUCTION;
        }
        if (!decode_modrm(buffer, buffer_size, &offset, inst, size, addr_size, NULL, &inst->op1)) return JD_ERR_INCOMPLETE_INSTRUCTION;
    } else if (opcode >= 0x80 && opcode <= 0x83) { // Grp1 (ALU r/m, imm)
        offset++;
        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        uint8_t modrm = buffer[offset];
        uint8_t reg = MODRM_REG(modrm);

        jd_mnem_t alu_map[] = { MNEM_ADD, MNEM_OR, MNEM_NONE, MNEM_NONE, MNEM_AND, MNEM_SUB, MNEM_XOR, MNEM_CMP };
        if (reg == 2 || reg == 3) return JD_ERR_INVALID_INSTRUCTION;
        inst->mnem = alu_map[reg];

        uint8_t size = (opcode == 0x81 || opcode == 0x83) ? op_size : 1;
        if (!decode_modrm(buffer, buffer_size, &offset, inst, size, addr_size, NULL, &inst->op1)) return JD_ERR_INCOMPLETE_INSTRUCTION;

        inst->op2.type = OP_IMM;
        uint8_t imm_size = (opcode == 0x81) ? (op_size == 8 ? 4 : op_size) : 1; // 0x81 with rex.w=1 is imm32 sign extended to 64
        inst->op2.size = imm_size; // store the actual imm size
        if (offset + imm_size > buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;

        if (imm_size == 1) {
            inst->op2.val.imm = (int8_t)buffer[offset++];
        } else if (imm_size == 2) {
            int16_t imm = 0; memcpy(&imm, buffer + offset, 2);
            inst->op2.val.imm = imm; offset += 2;
        } else if (imm_size == 4) {
            int32_t imm = 0; memcpy(&imm, buffer + offset, 4);
            inst->op2.val.imm = imm; offset += 4;
        }
    } else if (opcode == 0xC0 || opcode == 0xC1) { // Shift/Rotate imm8 (C0, C1)
        uint8_t size = (opcode == 0xC1) ? op_size : 1;
        offset++;
        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        uint8_t modrm = buffer[offset];
        uint8_t reg = MODRM_REG(modrm);

        if (reg == 4) inst->mnem = MNEM_SHL;
        else if (reg == 5) inst->mnem = MNEM_SHR;
        else if (reg == 7) inst->mnem = MNEM_SAR;
        else return JD_ERR_INVALID_INSTRUCTION; // Unsupported shift/rotate

        if (!decode_modrm(buffer, buffer_size, &offset, inst, size, addr_size, NULL, &inst->op1)) return JD_ERR_INCOMPLETE_INSTRUCTION;

        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        inst->op2.type = OP_IMM;
        inst->op2.val.imm = buffer[offset];
        inst->op2.size = 1;
        offset++;
    } else if ((opcode & 0xFC) == 0xD0) { // Shift/Rotate (D0, D1, D2, D3)
        uint8_t size = (opcode & 1) ? op_size : 1;
        offset++;
        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        uint8_t modrm = buffer[offset];
        uint8_t reg = MODRM_REG(modrm);

        if (reg == 4) inst->mnem = MNEM_SHL;
        else if (reg == 5) inst->mnem = MNEM_SHR;
        else if (reg == 7) inst->mnem = MNEM_SAR;
        else return JD_ERR_INVALID_INSTRUCTION; // Unsupported shift/rotate

        if (!decode_modrm(buffer, buffer_size, &offset, inst, size, addr_size, NULL, &inst->op1)) return JD_ERR_INCOMPLETE_INSTRUCTION;

        if ((opcode & 2) == 0) { // D0 / D1: by 1
            inst->op2.type = OP_IMM;
            inst->op2.val.imm = 1;
            inst->op2.size = 1;
        } else { // D2 / D3: by CL
            inst->op2.type = OP_REG;
            inst->op2.val.reg = REG_CL;
            inst->op2.size = 1;
        }
    } else if (opcode == 0xC6 || opcode == 0xC7) { // MOV r/m, imm
        offset++;
        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        uint8_t modrm = buffer[offset];
        if (MODRM_REG(modrm) != 0) return JD_ERR_INVALID_INSTRUCTION; // sub-opcode must be 0 for MOV
        inst->mnem = MNEM_MOV;
        uint8_t size = (opcode == 0xC7) ? op_size : 1;
        if (!decode_modrm(buffer, buffer_size, &offset, inst, size, addr_size, NULL, &inst->op1)) return JD_ERR_INCOMPLETE_INSTRUCTION;

        uint8_t imm_size = (opcode == 0xC7 && size == 8) ? 4 : size; // imm for 64-bit mov r/m, imm is 32-bit sign extended
        inst->op2.type = OP_IMM;
        inst->op2.size = imm_size;
        if (offset + imm_size > buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        if (imm_size == 1) inst->op2.val.imm = (int8_t)buffer[offset++];
        else if (imm_size == 2) { int16_t imm; memcpy(&imm, buffer + offset, 2); inst->op2.val.imm = imm; offset += 2; }
        else if (imm_size == 4) { int32_t imm; memcpy(&imm, buffer + offset, 4); inst->op2.val.imm = imm; offset += 4; }
    } else if (opcode == 0x84 || opcode == 0x85) { // TEST r/m, r
        inst->mnem = MNEM_TEST; offset++;
        uint8_t size = (opcode == 0x85) ? op_size : 1;
        if (!decode_modrm(buffer, buffer_size, &offset, inst, size, addr_size, &inst->op2, &inst->op1)) return JD_ERR_INCOMPLETE_INSTRUCTION;
    } else if (opcode == 0xF6 || opcode == 0xF7) { // Grp3 (TEST imm, NOT, NEG, MUL, IMUL, DIV, IDIV)
        offset++;
        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        uint8_t modrm = buffer[offset];
        uint8_t reg = MODRM_REG(modrm);
        uint8_t size = (opcode == 0xF7) ? op_size : 1;

        if (reg == 0) inst->mnem = MNEM_TEST;
        else if (reg == 2) inst->mnem = MNEM_NOT;
        else if (reg == 3) inst->mnem = MNEM_NEG;
        else if (reg == 4) inst->mnem = MNEM_MUL;
        else if (reg == 5) inst->mnem = MNEM_IMUL;
        else if (reg == 6) inst->mnem = MNEM_DIV;
        else if (reg == 7) inst->mnem = MNEM_IDIV;
        else return JD_ERR_INVALID_INSTRUCTION;

        if (!decode_modrm(buffer, buffer_size, &offset, inst, size, addr_size, NULL, &inst->op1)) return JD_ERR_INCOMPLETE_INSTRUCTION;

        if (reg == 0) { // TEST r/m, imm
            uint8_t imm_size = (size == 8) ? 4 : size;
            inst->op2.type = OP_IMM;
            inst->op2.size = imm_size;
            if (offset + imm_size > buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
            if (imm_size == 1) inst->op2.val.imm = (int8_t)buffer[offset++];
            else if (imm_size == 2) { int16_t imm; memcpy(&imm, buffer + offset, 2); inst->op2.val.imm = imm; offset += 2; }
            else if (imm_size == 4) { int32_t imm; memcpy(&imm, buffer + offset, 4); inst->op2.val.imm = imm; offset += 4; }
        }
    } else {
        return JD_ERR_INVALID_INSTRUCTION;
    }

    inst->length = offset;
    return JD_SUCCESS;
}
