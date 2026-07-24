#include <stdio.h>
#include <string.h>
#include <stdbool.h>
#include "jules_disasm.h"

int tests_run = 0;
int tests_passed = 0;
int tests_failed = 0;

void assert_eq_str(const char *expected, const char *actual, const char *msg) {
    tests_run++;
    if (strcmp(expected, actual) == 0) {
        tests_passed++;
        printf("[OK] %s\n", msg);
    } else {
        tests_failed++;
        printf("[FAIL] %s: Expected '%s', got '%s'\n", msg, expected, actual);
    }
}

void test_instr(const uint8_t *bytes, size_t size, const char *expected, const char *msg) {
    jd_instruction_t inst;
    jd_status_t st = jd_disassemble(bytes, size, 0x1000, &inst);
    if (st != JD_SUCCESS) {
        char err[64];
        snprintf(err, sizeof(err), "Error code %d", st);
        assert_eq_str(expected, err, msg);
    } else {
        assert_eq_str(expected, inst.formatted_string, msg);
    }
}

int main() {
    printf("Running disassembler tests...\n");

    // NOP
    uint8_t nop[] = { 0x90 };
    test_instr(nop, sizeof(nop), "nop", "NOP instruction");

    // RET
    uint8_t ret[] = { 0xC3 };
    test_instr(ret, sizeof(ret), "ret", "RET instruction");

    // MOV rax, 1
    uint8_t mov_rax_1[] = { 0x48, 0xC7, 0xC0, 0x01, 0x00, 0x00, 0x00 };
    test_instr(mov_rax_1, sizeof(mov_rax_1), "mov rax, 0x1", "MOV rax, imm32");

    // MOV rdi, rsi
    uint8_t mov_rdi_rsi[] = { 0x48, 0x89, 0xF7 };
    test_instr(mov_rdi_rsi, sizeof(mov_rdi_rsi), "mov rdi, rsi", "MOV rdi, rsi");

    // PUSH rbp
    uint8_t push_rbp[] = { 0x55 };
    test_instr(push_rbp, sizeof(push_rbp), "push rbp", "PUSH rbp");

    // POP rbp
    uint8_t pop_rbp[] = { 0x5D };
    test_instr(pop_rbp, sizeof(pop_rbp), "pop rbp", "POP rbp");

    // ADD rsp, 0x10
    uint8_t add_rsp_10[] = { 0x48, 0x83, 0xC4, 0x10 };
    test_instr(add_rsp_10, sizeof(add_rsp_10), "add rsp, 0x10", "ADD rsp, imm8");

    // MOV [rbp-4], edi
    uint8_t mov_mem_edi[] = { 0x89, 0x7D, 0xFC };
    test_instr(mov_mem_edi, sizeof(mov_mem_edi), "mov dword ptr [rbp-0x4], edi", "MOV [rbp-4], edi");

    // JMP rel8
    uint8_t jmp_rel8[] = { 0xEB, 0x0A }; // from 0x1000, length 2, + 10 = 0x100C
    test_instr(jmp_rel8, sizeof(jmp_rel8), "jmp 0x100c", "JMP rel8");

    // CALL rel32
    uint8_t call_rel32[] = { 0xE8, 0x00, 0x00, 0x00, 0x00 }; // next instruction
    test_instr(call_rel32, sizeof(call_rel32), "call 0x1005", "CALL rel32");

    // RIP relative LEA
    uint8_t lea_rip[] = { 0x48, 0x8D, 0x05, 0x12, 0x34, 0x00, 0x00 }; // LEA rax, [rip + 0x3412]
    test_instr(lea_rip, sizeof(lea_rip), "lea rax, [rip+0x3412]", "LEA rax, [rip+...]");

    // MOV al, 1
    uint8_t mov_al_1[] = { 0xB0, 0x01 };
    test_instr(mov_al_1, sizeof(mov_al_1), "mov al, 0x1", "MOV al, 1");

    // Real function: int add(int a, int b) { return a + b; }
    // 0:  55                      push   rbp
    // 1:  48 89 e5                mov    rbp,rsp
    // 4:  89 7d fc                mov    DWORD PTR [rbp-0x4],edi
    // 7:  89 75 f8                mov    DWORD PTR [rbp-0x8],esi
    // a:  8b 55 fc                mov    edx,DWORD PTR [rbp-0x4]
    // d:  8b 45 f8                mov    eax,DWORD PTR [rbp-0x8]
    // 10: 01 d0                   add    eax,edx
    // 12: 5d                      pop    rbp
    // 13: c3                      ret

    uint8_t real_func[] = {
        0x55, 0x48, 0x89, 0xE5, 0x89, 0x7D, 0xFC, 0x89, 0x75, 0xF8,
        0x8B, 0x55, 0xFC, 0x8B, 0x45, 0xF8, 0x01, 0xD0, 0x5D, 0xC3
    };

    printf("\nDisassembling real function:\n");
    size_t offset = 0;
    while (offset < sizeof(real_func)) {
        jd_instruction_t inst;
        jd_status_t st = jd_disassemble(real_func + offset, sizeof(real_func) - offset, 0x1000 + offset, &inst);
        if (st != JD_SUCCESS) {
            printf("Error %d at offset %zu\n", st, offset);
            tests_failed++;
            break;
        }
        printf("0x%lx: %s\n", 0x1000 + offset, inst.formatted_string);
        offset += inst.length;
    }

    if (offset == sizeof(real_func)) {
        printf("[OK] Real function disassembled completely\n");
        tests_passed++;
    } else {
        printf("[FAIL] Real function incomplete\n");
    }

    printf("\nTest Summary: %d run, %d passed, %d failed\n", tests_run + 1, tests_passed, tests_failed);

    return tests_failed > 0 ? 1 : 0;
}
