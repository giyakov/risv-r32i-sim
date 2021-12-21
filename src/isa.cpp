#include "isa.h"

#include <cassert>

namespace Sim {

template<InstructionType iType, CUALUOp aluOp, CUALUSrc src1, CUALUSrc src2>
static CUExecParams BuildALUInst()
{
    CUExecParams params = {};
    params.iType = iType;
    params.regWrite = true;
    params.aluSrc1 = src1;
    params.aluSrc2 = src2;
    params.aluOp = aluOp;
    params.resSrc = CUResSrc::ALU;
    params.isOpcodeOk = true;
    return params;
}

template<InstructionType iType, CUALUOp aluOp>
static CUExecParams BuildArithm()
{
    if (iType == InstructionType::R) {
        return BuildALUInst<iType, aluOp, CUALUSrc::REG, CUALUSrc::REG>();
    } else if (iType == InstructionType::I) {
        return BuildALUInst<iType, aluOp, CUALUSrc::REG, CUALUSrc::IMM>();
    }
    return CUExecParams{};
}

template<InstructionType iType, bool isJumpReg>
static CUExecParams BuildJump()
{
    CUExecParams params = {};
    params.regWrite = true;
    params.iType = iType;
    params.resSrc = CUResSrc::PC;
    params.isJump = true;
    params.isJumpReg = isJumpReg;
    params.isOpcodeOk = true;
    return params;
}

template<CUCmpOp cmpOp>
static CUExecParams BuildBranch()
{
    CUExecParams params = {};
    params.iType = InstructionType::B;
    params.cmpOp = cmpOp;
    params.isBranch = true;
    params.isOpcodeOk = true;
    return params;
}

template<CUMemOp memOp, bool signExt>
static CUExecParams BuildLoad()
{
    CUExecParams params = {};
    params.iType = InstructionType::I;
    params.regWrite = true;
    params.aluSrc1 = CUALUSrc::REG;
    params.aluSrc2 = CUALUSrc::IMM;
    params.aluOp = CUALUOp::ADD;
    params.resSrc = CUResSrc::MEM;
    params.memOp = memOp;
    params.memSignExt = signExt;
    params.isOpcodeOk = true;
    return params;
}

template<CUMemOp memOp>
static CUExecParams BuildStore()
{
    CUExecParams params = {};
    params.iType = InstructionType::S;
    params.aluSrc1 = CUALUSrc::REG;
    params.aluSrc2 = CUALUSrc::IMM;
    params.aluOp = CUALUOp::ADD;
    params.memWrite = true;
    params.memOp = memOp;
    params.memSignExt = false;
    params.isOpcodeOk = true;
    return params;
}

template<bool isInt>
static CUExecParams BuildSystem()
{
    CUExecParams params = {};
    params.iType = InstructionType::I;
    params.intpt = isInt;
    params.isOpcodeOk = true;
    return params;
}

ISAEntryDescription const isaDescription[] = {
    ISAEntryDescription{ "LUI",    ISAEntry::LUI,    Opcode::LUI,      InstructionType::U, 0b000, 0b0000000,
        BuildALUInst<InstructionType::U, CUALUOp::PASS_SRC2, CUALUSrc::UNKNOWN, CUALUSrc::IMM>() }, // 0
    ISAEntryDescription{ "AUIPC",  ISAEntry::AUIPC,  Opcode::AUIPC,    InstructionType::U, 0b000, 0b0000000, 
        BuildALUInst<InstructionType::U, CUALUOp::ADD, CUALUSrc::PC, CUALUSrc::IMM>() }, // 1
    ISAEntryDescription{ "JAL",    ISAEntry::JAL,    Opcode::JAL,      InstructionType::J, 0b000, 0b0000000, 
        BuildJump<InstructionType::J, false>() }, // 2
    ISAEntryDescription{ "JALR",   ISAEntry::JALR,   Opcode::JALR,     InstructionType::I, 0b000, 0b0000000, 
        BuildJump<InstructionType::I, true>() }, // 3
    ISAEntryDescription{ "BEQ",    ISAEntry::BEQ,    Opcode::BRANCH,   InstructionType::B, 0b000, 0b0000000, 
        BuildBranch<CUCmpOp::EQ>() }, // 4
    ISAEntryDescription{ "BNE",    ISAEntry::BNE,    Opcode::BRANCH,   InstructionType::B, 0b001, 0b0000000, 
        BuildBranch<CUCmpOp::NE>() }, // 5
    ISAEntryDescription{ "BLT",    ISAEntry::BLT,    Opcode::BRANCH,   InstructionType::B, 0b100, 0b0000000, 
        BuildBranch<CUCmpOp::LT>() }, // 6
    ISAEntryDescription{ "BGE",    ISAEntry::BGE,    Opcode::BRANCH,   InstructionType::B, 0b101, 0b0000000, 
        BuildBranch<CUCmpOp::GE>() }, // 7
    ISAEntryDescription{ "BLTU",   ISAEntry::BLTU,   Opcode::BRANCH,   InstructionType::B, 0b110, 0b0000000, 
        BuildBranch<CUCmpOp::LTU>() }, // 8
    ISAEntryDescription{ "BGEU",   ISAEntry::BGEU,   Opcode::BRANCH,   InstructionType::B, 0b111, 0b0000000, 
        BuildBranch<CUCmpOp::GEU>() }, // 9
    ISAEntryDescription{ "LB",     ISAEntry::LB,     Opcode::LOAD,     InstructionType::I, 0b000, 0b0000000,
        BuildLoad<CUMemOp::BYTE, true>()}, // 10
    ISAEntryDescription{ "LH",     ISAEntry::LH,     Opcode::LOAD,     InstructionType::I, 0b001, 0b0000000,
        BuildLoad<CUMemOp::HALF, true>()}, // 11
    ISAEntryDescription{ "LW",     ISAEntry::LW,     Opcode::LOAD,     InstructionType::I, 0b010, 0b0000000, 
        BuildLoad<CUMemOp::WORD, false>() }, // 12
    ISAEntryDescription{ "LBU",    ISAEntry::LBU,    Opcode::LOAD,     InstructionType::I, 0b100, 0b0000000, 
        BuildLoad<CUMemOp::BYTE, false>() }, // 13
    ISAEntryDescription{ "LHU",    ISAEntry::LHU,    Opcode::LOAD,     InstructionType::I, 0b101, 0b0000000,
        BuildLoad<CUMemOp::HALF, false>()}, // 14
    ISAEntryDescription{ "SB",     ISAEntry::SB,     Opcode::STORE,    InstructionType::S, 0b000, 0b0000000,
        BuildStore<CUMemOp::BYTE>() }, // 15
    ISAEntryDescription{ "SH",     ISAEntry::SH,     Opcode::STORE,    InstructionType::S, 0b001, 0b0000000,
        BuildStore<CUMemOp::HALF>() }, // 16
    ISAEntryDescription{ "SW",     ISAEntry::SW,     Opcode::STORE,    InstructionType::S, 0b010, 0b0000000, 
        BuildStore<CUMemOp::WORD>() }, // 17
    ISAEntryDescription{ "ADDI",   ISAEntry::ADDI,   Opcode::OP_IMM,   InstructionType::I, 0b000, 0b0000000, 
        BuildArithm<InstructionType::I, CUALUOp::ADD>() }, // 18
    ISAEntryDescription{ "SLTI",   ISAEntry::SLTI,   Opcode::OP_IMM,   InstructionType::I, 0b010, 0b0000000,
        BuildArithm<InstructionType::I, CUALUOp::SLT>() }, // 19
    ISAEntryDescription{ "SLTIU",  ISAEntry::SLTIU,  Opcode::OP_IMM,   InstructionType::I, 0b011, 0b0000000, 
        BuildArithm<InstructionType::I, CUALUOp::SLTU>() }, // 20
    ISAEntryDescription{ "XORI",   ISAEntry::XORI,   Opcode::OP_IMM,   InstructionType::I, 0b100, 0b0000000, 
        BuildArithm<InstructionType::I, CUALUOp::XOR>() }, // 21
    ISAEntryDescription{ "ORI",    ISAEntry::ORI,    Opcode::OP_IMM,   InstructionType::I, 0b110, 0b0000000, 
        BuildArithm<InstructionType::I, CUALUOp::OR>() }, // 22
    ISAEntryDescription{ "ANDI",   ISAEntry::ANDI,   Opcode::OP_IMM,   InstructionType::I, 0b111, 0b0000000, 
        BuildArithm<InstructionType::I, CUALUOp::AND>() }, // 23
    ISAEntryDescription{ "SLLI",   ISAEntry::SLLI,   Opcode::OP_IMM,   InstructionType::I, 0b001, 0b0000000, 
        BuildArithm<InstructionType::I, CUALUOp::SLL>() }, // 24
    ISAEntryDescription{ "SRLI",   ISAEntry::SRLI,   Opcode::OP_IMM,   InstructionType::I, 0b101, 0b0000000, 
        BuildArithm<InstructionType::I, CUALUOp::SRL>() }, // 25
    ISAEntryDescription{ "SRAI",   ISAEntry::SRAI,   Opcode::OP_IMM,   InstructionType::I, 0b101, 0b0100000, 
        BuildArithm<InstructionType::I, CUALUOp::SRA>() }, // 26
    ISAEntryDescription{ "ADD",    ISAEntry::ADD,    Opcode::OP,       InstructionType::R, 0b000, 0b0000000, 
        BuildArithm<InstructionType::R, CUALUOp::ADD>() }, // 27
    ISAEntryDescription{ "SUB",    ISAEntry::SUB,    Opcode::OP,       InstructionType::R, 0b000, 0b0100000, 
        BuildArithm<InstructionType::R, CUALUOp::SUB>() }, // 28
    ISAEntryDescription{ "SLL",    ISAEntry::SLL,    Opcode::OP,       InstructionType::R, 0b001, 0b0000000, 
        BuildArithm<InstructionType::R, CUALUOp::SLL>() }, // 29
    ISAEntryDescription{ "SLT",    ISAEntry::SLT,    Opcode::OP,       InstructionType::R, 0b010, 0b0000000,
        BuildArithm<InstructionType::R, CUALUOp::SLT>() }, // 30
    ISAEntryDescription{ "SLTU",   ISAEntry::SLTU,   Opcode::OP,       InstructionType::R, 0b011, 0b0000000, 
        BuildArithm<InstructionType::R, CUALUOp::SLTU>() }, // 31
    ISAEntryDescription{ "XOR",    ISAEntry::XOR,    Opcode::OP,       InstructionType::R, 0b100, 0b0000000, 
        BuildArithm<InstructionType::R, CUALUOp::XOR>() }, // 32
    ISAEntryDescription{ "SRL",    ISAEntry::SRL,    Opcode::OP,       InstructionType::R, 0b101, 0b0000000, 
        BuildArithm<InstructionType::R, CUALUOp::SRL>() }, // 33
    ISAEntryDescription{ "SRA",    ISAEntry::SRA,    Opcode::OP,       InstructionType::R, 0b101, 0b0100000,
        BuildArithm<InstructionType::R, CUALUOp::SRA>() }, // 34
    ISAEntryDescription{ "OR",     ISAEntry::OR,     Opcode::OP,       InstructionType::R, 0b110, 0b0000000, 
        BuildArithm<InstructionType::R, CUALUOp::OR>() }, // 35
    ISAEntryDescription{ "AND",    ISAEntry::AND,    Opcode::OP,       InstructionType::R, 0b111, 0b0000000, 
        BuildArithm<InstructionType::R, CUALUOp::AND>() }, // 36
    ISAEntryDescription{ "FENCE",  ISAEntry::FENCE,  Opcode::MISC_MEM, InstructionType::I, 0b000, 0b0000000,
        BuildSystem<false>() }, // 37
    ISAEntryDescription{ "ECALL",  ISAEntry::ECALL,  Opcode::SYSTEM,   InstructionType::I, 0b000, 0b0000000,
        BuildSystem<true>() }, // 38
    ISAEntryDescription{ "EBREAK", ISAEntry::EBREAK, Opcode::SYSTEM, InstructionType::I, 0b000, 0b0000000,
        BuildSystem<true>() }, // 39
    ISAEntryDescription{ "UNKNOWN",ISAEntry::UNKNOWN,Opcode::UNKNOWN,  InstructionType::UNKNOWN_TYPE, 0, 0,
        CUExecParams{ .isOpcodeOk = false }} // 40
};

 ISAEntryDescription const &UnpackISAEntryDescription(Instruction instr)
{
    switch ((Opcode)instr.rType.opcode) {
        case Opcode::LUI:   return isaDescription[0];
        case Opcode::AUIPC: return isaDescription[1];
        case Opcode::JAL:   return isaDescription[2];
        case Opcode::JALR:  return isaDescription[3];
        case Opcode::BRANCH:
            switch (instr.rType.funct3) {
                case 0b000: return isaDescription[4];
                case 0b001: return isaDescription[5];
                case 0b100: return isaDescription[6];
                case 0b101: return isaDescription[7];
                case 0b110: return isaDescription[8];
                case 0b111: return isaDescription[9];
                default: return isaDescription[40];
            }
        case Opcode::LOAD:
            switch (instr.rType.funct3) {
                case 0b000: return isaDescription[10];
                case 0b001: return isaDescription[11];
                case 0b010: return isaDescription[12];
                case 0b100: return isaDescription[13];
                case 0b101: return isaDescription[14];
                default: return isaDescription[40];
            }
        case Opcode::STORE:
            switch (instr.rType.funct3) {
                case 0b000: return isaDescription[15];
                case 0b001: return isaDescription[16];
                case 0b010: return isaDescription[17];
                default: return isaDescription[40];
            }
        case Opcode::OP_IMM:
            switch (instr.rType.funct3) {
                case 0b000: return isaDescription[18];
                case 0b010: return isaDescription[19];
                case 0b011: return isaDescription[20];
                case 0b100: return isaDescription[21];
                case 0b110: return isaDescription[22];
                case 0b111: return isaDescription[23];
                case 0b001: return isaDescription[24];
                case 0b101:
                    switch (instr.rType.funct7) {
                        case 0b0000000: return isaDescription[25];
                        case 0b0100000: return isaDescription[26];
                        default: return isaDescription[40];
                    }
                default: return isaDescription[40];
            }
        case Opcode::OP:
            switch (instr.rType.funct3) {
                case 0b000:
                    switch (instr.rType.funct7) {
                        case 0b0000000: return isaDescription[27];
                        case 0b0100000: return isaDescription[28];
                        default: return isaDescription[40];
                    }
                case 0b001: return isaDescription[29];
                case 0b010: return isaDescription[30];
                case 0b011: return isaDescription[31];
                case 0b100: return isaDescription[32];
                case 0b101:
                    switch (instr.rType.funct7) {
                        case 0b0000000: return isaDescription[33];
                        case 0b0100000: return isaDescription[34];
                        default: return isaDescription[40];
                    }
                case 0b110: return isaDescription[35];
                case 0b111: return isaDescription[36];
                default: return isaDescription[40];
            }
        case Opcode::MISC_MEM: return isaDescription[37];
        case Opcode::SYSTEM:
            switch (instr.iType.imm11_0) {
                case 0b000000000000: return isaDescription[38];
                case 0b000000000001: return isaDescription[39];
                default: return isaDescription[40];
            }
        default: return isaDescription[40];
    }
}

} // namespace Sim
