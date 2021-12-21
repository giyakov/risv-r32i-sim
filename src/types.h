#ifndef SIM_TYPES_H
#define SIM_TYPES_H

#include <cstdint>
#include <tick_module.h>

using i8_t = std::int8_t;
using u8_t = std::uint8_t;

using i16_t = std::int16_t;
using u16_t = std::uint16_t;

using i32_t = std::int32_t;
using u32_t = std::uint32_t;

namespace Sim {

enum class InstructionType : u8_t {
    R, I, S, B, U, J, UNKNOWN_TYPE
};

union Instruction {
    u32_t raw;
    struct RTypeInstruction final {
        u32_t opcode : 7;
        u32_t rd : 5;
        u32_t funct3 : 3;
        u32_t rs1 : 5;
        u32_t rs2 : 5;
        u32_t funct7 : 7;
    } rType;
    struct ITypeInstruction final {
        u32_t opcode : 7;
        u32_t rd : 5;
        u32_t funct3 : 3;
        u32_t rs1 : 5;
        u32_t imm11_0 : 12;
    } iType;
    struct STypeInstruction final {
        u32_t opcode : 7;
        u32_t imm4_0 : 5;
        u32_t funct3 : 3;
        u32_t rs1 : 5;
        u32_t rs2 : 5;
        u32_t imm11_5 : 7;
    } sType;
    struct BTypeInstruction final {
        u32_t opcode : 7;
        u32_t imm11 : 1;
        u32_t imm4_1 : 4;
        u32_t funct3 : 3;
        u32_t rs1 : 5;
        u32_t rs2 : 5;
        u32_t imm10_5 : 6;
        u32_t imm12 : 1;
    } bType;
    struct UTypeInstruction final {
        u32_t opcode : 7;
        u32_t rd : 5;
        u32_t imm31_12 : 20;
    } uType;
    struct JTypeInstruction final {
        u32_t opcode : 7;
        u32_t rd : 5;
        u32_t imm19_12 : 8;
        u32_t imm11 : 1;
        u32_t imm10_1 : 10;
        u32_t imm20 : 1;
    } jType;
};
static_assert(sizeof(Instruction) == sizeof(u32_t));

union Immediate {
    u32_t raw;
    struct ITypeImmediate final {
        u32_t inst20 : 1;
        u32_t inst24_21 : 4;
        u32_t inst30_25 : 6;
        u32_t inst31 : 21;
    } iType;
    struct STypeImmediate final {
        u32_t inst7 : 1;
        u32_t inst11_8 : 4;
        u32_t inst30_25 : 6;
        u32_t inst31 : 21;
    } sType;
    struct BTypeImmediate final {
        u32_t pad : 1;
        u32_t inst11_8 : 4;
        u32_t inst30_25 : 6;
        u32_t inst7 : 1;
        u32_t inst31 : 20;
    } bType;
    struct UTypeImmediate final {
        u32_t pad : 12;
        u32_t inst19_12 : 8;
        u32_t inst30_20 : 11;
        u32_t inst31 : 1;
    } uType;
    struct JTypeImmediate final {
        u32_t pad : 1;
        u32_t inst24_21 : 4;
        u32_t inst30_25 : 6;
        u32_t inst20 : 1;
        u32_t inst19_12 : 8;
        u32_t inst31 : 11;
    } jType;
};
static_assert(sizeof(Immediate) == sizeof(u32_t));

enum class CUResSrc : u8_t {
    ALU, MEM, PC,
    UNKNOWN
};

enum class CUALUSrc : u8_t {
    REG, IMM, PC,
    UNKNOWN
};

enum class CUALUOp : u8_t {
    ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND, PASS_SRC2,
    UNKNOWN
};

enum class CUMemOp : u8_t {
    WORD, HALF, BYTE,
    UNKNOWN
};

enum class CUCmpOp : u8_t {
    EQ, NE, LT, GE, LTU, GEU,
    UNKNOWN
};

struct CUExecParams final {
    InstructionType iType = InstructionType::R;
    bool regWrite = false;

    CUALUSrc aluSrc1 = CUALUSrc::REG;
    CUALUSrc aluSrc2 = CUALUSrc::REG;
    CUALUOp aluOp = CUALUOp::PASS_SRC2;

    CUCmpOp cmpOp = CUCmpOp::EQ;
    bool isJump = false;
    bool isJumpReg = false;
    bool isBranch = false;

    CUMemOp memOp = CUMemOp::WORD;
    bool memWrite = false;
    bool memSignExt = false;

    CUResSrc resSrc = CUResSrc::ALU;

    bool isOpcodeOk = true;

    bool intpt = false;
};

} // namespace Sim

#endif // SIM_TYPES_H
