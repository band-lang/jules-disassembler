#include "jules_disasm.h"
#include "decoder.h"
#include "formatter.h"

jd_status_t jd_disassemble(const uint8_t *buffer, size_t buffer_size, uint64_t address, jd_instruction_t *out_instr) {
    if (out_instr == NULL) return JD_ERR_NULL_POINTER;

    jd_decoded_inst_t inst;
    jd_status_t status = decode_instruction(buffer, buffer_size, &inst);
    if (status != JD_SUCCESS) return status;

    out_instr->length = inst.length;
    out_instr->address = address;

    format_instruction(&inst, address, out_instr->formatted_string, sizeof(out_instr->formatted_string));

    return JD_SUCCESS;
}
