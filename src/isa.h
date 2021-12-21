#ifndef SIM_ISA_H
#define SIM_ISA_H

#include <types.h>

namespace Sim {

enum class Opcode : u8_t {
    LUI         = 0b0110111,
    AUIPC       = 0b0010111,
    JAL         = 0b1101111,
    JALR        = 0b1100111,
    BRANCH      = 0b1100011,
    LOAD        = 0b0000011,
    STORE       = 0b0100011,
    OP_IMM      = 0b0010011,
    OP          = 0b0110011,
    MISC_MEM    = 0b0001111,
    SYSTEM      = 0b1110011,
    UNKNOWN
};

enum class ISAEntry : u8_t {
    LUI, AUIPC, JAL, JALR, BEQ, BNE, BLT, BGE, BLTU, BGEU, LB, LH, LW, LBU, LHU, SB, SH, SW, ADDI, SLTI, SLTIU,
    XORI, ORI, ANDI, SLLI, SRLI, SRAI, ADD, SUB, SLL, SLT, SLTU, XOR, SRL, SRA, OR, AND, FENCE, ECALL, EBREAK,
    UNKNOWN
};

struct ISAEntryDescription final {
    char const *asmStr = nullptr;
    ISAEntry isaEntry = ISAEntry::UNKNOWN;
    Opcode opcode = Opcode::UNKNOWN;
    InstructionType iType = InstructionType::UNKNOWN_TYPE;
    u8_t funct3 = 0;
    u8_t funct7 = 0;

    CUExecParams execParams = {};
};

extern ISAEntryDescription const isaDescription[];

ISAEntryDescription const &UnpackISAEntryDescription(Instruction instr);

} // namespace Sim

#endif // SIM_ISA_H
