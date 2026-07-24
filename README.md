# jules-disassembler

## Project Goal
Implement a production-quality x86-64 disassembler library in pure C, with full test coverage and clean architecture. This is NOT a minimal prototype. This is a serious engineering project.

## Architecture Requirements

### Library Structure
Organize the code into multiple files with clear separation of concerns. The library must be split into modules, not a single monolithic file:

- Instruction decoder: reads raw bytes and identifies the instruction (opcode, prefixes, operands)
- Operand formatter: converts decoded operands into human-readable strings
- Instruction formatter: combines opcode mnemonic with formatted operands into the final assembly string
- Public API: clean header file exposing only what users need

### Feature Requirements
The disassembler must decode a substantial subset of x86-64, covering real-world code:

- All common integer instructions: MOV, ADD, SUB, AND, OR, XOR, CMP, INC, DEC, MUL, DIV, PUSH, POP, CALL, RET, JMP, JE, JNE, JL, JG, JLE, JGE, NOP, LEA
- All common addressing modes: register-to-register, register-to-memory, memory-to-register, immediate-to-register, immediate-to-memory
- Support for all 16 general-purpose registers in 64-bit mode: RAX, RBX, RCX, RDX, RSI, RDI, RBP, RSP, R8-R15
- REX prefixes for 64-bit operand size and extended registers
- ModR/M and SIB byte decoding for memory operands
- Basic RIP-relative addressing (used in position-independent code)

### Technical Constraints
- Pure C (C11 standard, compiling with `-Wall -Wextra -pedantic` without warnings)
- Zero external dependencies beyond the standard C library
- Clean modular architecture with header files and multiple implementation files
- No memory leaks under Valgrind
- Build system: plain Makefile or CMakeLists.txt, your choice, just make it work with `make && ./test`

## Testing Requirements

### Self-Testing Binary
Create a comprehensive test suite that covers:

- Every supported instruction, tested in isolation
- Every supported addressing mode for MOV
- Round-trip correctness: assemble known bytes (defined as hex arrays in tests), disassemble them, verify the output string matches expected
- Edge cases: empty input, invalid opcodes, truncated input (instruction extends beyond buffer)
- A test that disassembles a small real function (provide a hardcoded hex array of a simple C function compiled with `-O0`)

The test binary must return 0 on success and nonzero on any failure, suitable for automated CI.

## Documentation Requirements
The README.md must contain:
- Build instructions
- Supported instruction set table
- Architecture overview explaining how the decoder works
- Known limitations

## Workflow
- Create a branch named `dev` and do all work there
- Commit regularly with meaningful commit messages
- When the project is complete and all tests pass, open a pull request from `dev` into `main`
- Work completely autonomously. Do not wait for approval. Do not ask questions. Implement, test, document, and submit.
