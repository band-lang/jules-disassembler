import sys

content = open("src/decoder.c", "r").read()

content = content.replace("    } else if (opcode == 0x0F) { // 2-byte opcodes\n        offset++;\n        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;\n        uint8_t op2 = buffer[offset++];", """    } else if (opcode == 0x0F) { // 2-byte opcodes
        offset++;
        if (offset >= buffer_size) return JD_ERR_INCOMPLETE_INSTRUCTION;
        uint8_t op2 = buffer[offset++];
        if (op2 == 0x05) { // SYSCALL
            inst->mnem = MNEM_SYSCALL;
            inst->length = offset;
            return JD_SUCCESS;
        }""")

with open("src/decoder.c", "w") as f:
    f.write(content)

content_h = open("src/decoder.h", "r").read()
content_h = content_h.replace("MNEM_HLT", "MNEM_HLT, MNEM_SYSCALL")
with open("src/decoder.h", "w") as f:
    f.write(content_h)
