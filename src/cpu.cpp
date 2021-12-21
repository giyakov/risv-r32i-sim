#include "cpu.h"
#include <cassert>

namespace Sim {

void CPU::Tick()
{
    writebackStage.Tick(*this);
    memoryStage.Tick(*this);
    executeStage.Tick(*this);
    decodeStage.Tick(*this);
    fetchStage.Tick(*this);
    huModule.Tick(*this);
}

void CPU::Execute()
{
    while (!shutdown) {
        Tick();
    }
}

void HUModule::Raise(HUExcecutionStage stage, HUExceptionType type, u32_t pc)
{
    if ((u8_t)stage < (u8_t)exceptionExecStage) {
        return;
    }

    exceptionExecStage = stage;
    exceptionPC = pc;
    exceptionType = type;
}

void HUModule::Tick(CPU &cpu)
{
    auto &feState = cpu.fetchStage.state;
    auto &deState = cpu.decodeStage.state;
    auto &exState = cpu.executeStage.state;
    auto &memState = cpu.memoryStage.state;
    auto &wbState = cpu.writebackStage.state;

    bool loadHazard = (exState.read.execParams.resSrc == CUResSrc::MEM) &&
        ((exState.read.rda == deState.read.inst.rType.rs1) ||
        (exState.read.rda == deState.read.inst.rType.rs2));

    bool pcFlush = cpu.executeStage.pcR;

    if ((pcFlush || loadHazard) && (u8_t)exceptionType <= (u8_t)HUExcecutionStage::DECODE) {
        exceptionExecStage = HUExcecutionStage::NONE;
    }

    if ((u8_t)exceptionExecStage >= (u8_t)HUExcecutionStage::MEMORY) {
        cpu.writebackStage.state.write.regWrite = false;
    }
    cpu.writebackStage.state.Tick();

    if ((u8_t)exceptionExecStage >= (u8_t)HUExcecutionStage::EXECUTE) {
        memState.write.execParams.regWrite = false;
        memState.write.execParams.memWrite = false;
        memState.write.execParams.resSrc = CUResSrc::ALU;
    }
    memState.Tick();

    if ((u8_t)exceptionExecStage >= (u8_t)HUExcecutionStage::DECODE || loadHazard || pcFlush) {
        exState.write.execParams.regWrite = false;
        exState.write.execParams.memWrite = false;
        exState.write.execParams.resSrc = CUResSrc::ALU;
        exState.write.execParams.isBranch = false;
        exState.write.execParams.isJump = false;
        exState.write.execParams.intpt = false;
    }
    exState.Tick();

    if ((u8_t)exceptionExecStage >= (u8_t)HUExcecutionStage::FETCH || pcFlush) {
        deState.write.v = true;
        deState.Tick();
    } else if (!loadHazard) {
        deState.Tick();
    }

    if ((u8_t)exceptionExecStage > (u8_t)HUExcecutionStage::NONE) {
        feState.write.pc = cpu.tvec;
        feState.Tick();
    } else if (!loadHazard) {
        feState.Tick();
    }

    exceptionExecStage = HUExcecutionStage::NONE;
}

HURS HUModule::GetRS(CPU& cpu, u8_t rsa)
{
    if (cpu.memoryStage.state.read.execParams.regWrite && (rsa == cpu.memoryStage.state.read.regAddr)) {
        return HURS::BP_MEM;
    }

    if (cpu.writebackStage.state.read.regWrite && (rsa == cpu.writebackStage.state.read.regAddr)) {
        return HURS::BP_WB;
    }

    return HURS::REG;
}

HUExceptionType MMU::Load(CPU &cpu, u32_t a, u32_t *dst, CUMemOp memOp)
{
    if (a % 4) {
        return HUExceptionType::UNALIGNED_ADDR;
    }
    if (a >= std::size(memory) * sizeof(u32_t)) {
        return HUExceptionType::MMU_MISS;
    }

    *dst = memory[a / 4];
    return HUExceptionType::NONE;
}

HUExceptionType MMU::Store(CPU &cpu, u32_t a, u32_t data, CUMemOp memOp)
{
    if (a % 4) {
        return HUExceptionType::UNALIGNED_ADDR;
    }
    if (a >= std::size(memory) * sizeof(u32_t)) {
        return HUExceptionType::MMU_MISS;
    }
    if (a == 0) {
        cpu.shutdown = true;
    }

    memory[a / 4] = data;
    return HUExceptionType::NONE;
}

void FetchStage::Tick(CPU &cpu)
{
    if (auto excType = cpu.mmu.Load(cpu, state.read.pc, &cpu.decodeStage.state.write.inst.raw);
        excType != HUExceptionType::NONE) {
        cpu.huModule.Raise(HUExcecutionStage::FETCH, excType, state.read.pc);
    }

    u32_t pcNext = 0;
    if (!cpu.executeStage.pcR) {
        pcNext = state.read.pc + 4;
    } else {
        pcNext = cpu.executeStage.jumpBase + cpu.executeStage.state.read.immExt;
    }

    state.write.pc = pcNext;

    cpu.decodeStage.state.write.v = 0; // hu
    cpu.decodeStage.state.write.pc = state.read.pc;
    cpu.decodeStage.state.write.pcNext = pcNext;
}

void DecodeStage::Regfile::Tick(CPU &cpu)
{
    u8_t a1 = cpu.decodeStage.state.read.inst.rType.rs1;
    u8_t a2 = cpu.decodeStage.state.read.inst.rType.rs2;
    u8_t a3 = cpu.writebackStage.state.read.regAddr;
    u32_t d3 = cpu.writebackStage.state.read.regWdata;
    u8_t we3 = cpu.writebackStage.state.read.regWrite;

    if (we3) {
        gpr[a3] = d3;
        gpr[0] = 0;
    }

    cpu.executeStage.state.write.rs1v = gpr[a1];
    cpu.executeStage.state.write.rs2v = gpr[a2];
}

void DecodeStage::Tick(CPU &cpu)
{
    auto inst = state.read.inst;

    CUExecParams params = DecodeInstruction(inst);
    CUExecParams &outParams = cpu.executeStage.state.write.execParams;

    outParams = params;
    outParams.regWrite = params.regWrite && !state.read.v;
    outParams.resSrc = state.read.v ? CUResSrc::ALU : params.resSrc;
    outParams.memWrite = params.memWrite && !state.read.v;
    outParams.isJump = params.isJump && !state.read.v;
    outParams.isBranch = params.isBranch && !state.read.v;
    outParams.intpt = params.intpt && !state.read.v;

    if (!params.isOpcodeOk && !state.read.v) {
        cpu.huModule.Raise(HUExcecutionStage::DECODE, HUExceptionType::BAD_OPCODE, state.read.pc);
    }

    cpu.executeStage.state.write.immExt = UnpackImmediate(inst, params.iType);
    cpu.executeStage.state.write.pc = state.read.pc;
    cpu.executeStage.state.write.pcNext = state.read.pcNext;
    cpu.executeStage.state.write.rs1a = inst.rType.rs1;
    cpu.executeStage.state.write.rs2a = inst.rType.rs2;
    cpu.executeStage.state.write.rda = inst.rType.rd;

    regfile.Tick(cpu);
}

u32_t DecodeStage::UnpackImmediate(Instruction inst, InstructionType iType)
{
    bool signBit = (inst.raw >> 31) & 1;
    Immediate imm = { .raw = 0 };

    switch (iType) {
        case InstructionType::R: {
            break;
        }
        case InstructionType::I: {
            imm.raw = inst.iType.imm11_0;
            if (signBit) {
                imm.iType.inst31 = (u32_t)(-1);
            }
            break;
        }
        case InstructionType::S: {
            imm.raw = inst.sType.imm4_0;
            imm.sType.inst30_25 = inst.sType.imm11_5;
            if (signBit) {
                imm.sType.inst31 = (u32_t)(-1);
            }
            break;
        }
        case InstructionType::B: {
            imm.bType.inst11_8 = inst.bType.imm4_1;
            imm.bType.inst30_25 = inst.bType.imm10_5;
            imm.bType.inst7 = inst.bType.imm11;
            if (signBit) {
                imm.bType.inst31 = (u32_t)(-1);
            }
            break;
        }
        case InstructionType::U: {
            imm.uType.inst19_12 = inst.uType.imm31_12;
            break;
        }
        case InstructionType::J: {
            imm.raw = inst.jType.imm10_1 << 1;
            imm.jType.inst20 = inst.jType.imm11;
            imm.jType.inst19_12 = inst.jType.imm19_12;
            if (signBit) {
                imm.jType.inst31 = (u32_t)(-1);
            }
            break;
        }
        default: assert(!"Unexpected instruction type");
    };

    return imm.raw;
}

CUExecParams DecodeStage::DecodeInstruction(Instruction inst)
{
    auto const &desc = UnpackISAEntryDescription(inst);
    return desc.execParams;
}

void ExecuteStage::Tick(CPU &cpu)
{
    cpu.memoryStage.state.write.execParams.regWrite = state.read.execParams.regWrite;
    cpu.memoryStage.state.write.execParams.memWrite = state.read.execParams.memWrite;
    cpu.memoryStage.state.write.execParams.memOp = state.read.execParams.memOp;
    cpu.memoryStage.state.write.execParams.memSignExt = state.read.execParams.memSignExt;

    cpu.memoryStage.state.write.execParams.resSrc = state.read.execParams.resSrc;
    cpu.memoryStage.state.write.regAddr = state.read.rda;

    auto huRSSwitch = [&cpu](HURS huRS, u32_t rsv) {
        switch (huRS) {
            case HURS::REG:
                return rsv;
            case HURS::BP_MEM:
                return cpu.memoryStage.state.read.aluRes;
            case HURS::BP_WB:
                return cpu.writebackStage.state.read.regWdata;
            default: assert(!"Unexpected value for HU_RS");
        }
    };

    u32_t sv1 = huRSSwitch(cpu.huModule.GetRS(cpu, state.read.rs1a), state.read.rs1v);
    u32_t sv2 = huRSSwitch(cpu.huModule.GetRS(cpu, state.read.rs2a), state.read.rs2v);
    jumpBase = state.read.execParams.isJumpReg ? (sv1 & ~(u32_t)1) : state.read.pc;

    cpu.memoryStage.state.write.memWdata = sv2;

    if (state.read.execParams.aluSrc1 == CUALUSrc::PC) {
        sv1 = state.read.pc;
    }
    if (state.read.execParams.aluSrc2 == CUALUSrc::IMM) {
        sv2 = state.read.immExt;
    }

    u32_t aluRes = ALUOperator(state.read.execParams.aluOp, sv1, sv2);
    cpu.memoryStage.state.write.aluRes = aluRes;

    bool cmpRes = CMPOperator(state.read.execParams.cmpOp, sv1, sv2);

    pcR = state.read.execParams.isJump ||
        (state.read.execParams.isBranch && cmpRes);

    cpu.memoryStage.state.write.pcNext = state.read.pcNext;
    cpu.memoryStage.state.write.pc = state.read.pc;

    if (state.read.execParams.intpt) {
        cpu.huModule.Raise(HUExcecutionStage::EXECUTE, HUExceptionType::INT, state.read.pc);
    }
}

u32_t ExecuteStage::ALUOperator(CUALUOp op, u32_t rs1v, u32_t rs2v)
{
    switch (op) {
        case CUALUOp::ADD:
            return rs1v + rs2v;
        case CUALUOp::SUB:
            return rs1v - rs2v;
        case CUALUOp::SLL:
            return rs1v << (rs2v & 31);
        case CUALUOp::SLT:
            return !!((i32_t)rs1v < (i32_t)rs2v);
        case CUALUOp::SLTU:
            return !!(rs1v < rs2v);
        case CUALUOp::XOR:
            return rs1v ^ rs2v;
        case CUALUOp::SRL:
            return rs1v >> (rs2v & 31);
        case CUALUOp::SRA:
            return (i32_t)rs1v >> (rs2v & 31);
        case CUALUOp::OR:
            return rs1v | rs2v;
        case CUALUOp::AND:
            return rs1v & rs2v;
        case CUALUOp::PASS_SRC2:
            return rs2v;
        default: assert(!"Unexpected ALU operation");
    };
}

bool ExecuteStage::CMPOperator(CUCmpOp op, u32_t rs1v, u32_t rs2v)
{
    switch (op) {
        case CUCmpOp::EQ:
            return rs1v == rs2v;
        case CUCmpOp::NE:
            return rs1v != rs2v;
        case CUCmpOp::LT:
            return (i32_t)rs1v < (i32_t)rs2v;
        case CUCmpOp::GE:
            return (i32_t)rs1v >= (i32_t)rs2v;
        case CUCmpOp::LTU:
            return rs1v < rs2v;
        case CUCmpOp::GEU:
            return rs1v >= rs2v;
        default: assert(!"Unexpected CMP operation");
    };
}

void MemoryStage::Tick(CPU &cpu)
{
    u32_t mmuRD = 0;

    if (state.read.execParams.resSrc == CUResSrc::MEM) {
        if (auto ex = cpu.mmu.Load(cpu, state.read.aluRes & (~(u32_t)3), &mmuRD); ex != HUExceptionType::NONE) {
            cpu.huModule.Raise(HUExcecutionStage::MEMORY, ex, state.read.pc);
        }

        u8_t sh = state.read.aluRes & ((u32_t)3);
        u8_t align = 4;
        mmuRD >>= sh;

        switch (state.read.execParams.memOp) {
            case CUMemOp::BYTE:
                mmuRD = state.read.execParams.memSignExt ? (i32_t)(i8_t)mmuRD : (u8_t)mmuRD;
                align = 1;
                break;
            case CUMemOp::HALF:
                mmuRD = state.read.execParams.memSignExt ? (i32_t)(i16_t)mmuRD : (u16_t)mmuRD;
                align = 2;
                break;
            case CUMemOp::WORD:
                align = 4;
                break;
            default: assert(!"Unexpected memory operation");
        }
        if (sh % align) {
            cpu.huModule.Raise(HUExcecutionStage::MEMORY, HUExceptionType::UNALIGNED_ADDR,
                state.read.pc);
        }
    }

    if (state.read.execParams.memWrite) {
        if (auto ex = cpu.mmu.Store(cpu, state.read.aluRes, state.read.memWdata, state.read.execParams.memOp);
            ex != HUExceptionType::NONE) {
            cpu.huModule.Raise(HUExcecutionStage::MEMORY, ex, state.read.pc);
        }
    }

    switch (state.read.execParams.resSrc) {
        case CUResSrc::ALU:
            cpu.writebackStage.state.write.regWdata = state.read.aluRes;
            break;
        case CUResSrc::MEM:
            cpu.writebackStage.state.write.regWdata = mmuRD;
            break;
        case CUResSrc::PC:
            cpu.writebackStage.state.write.regWdata = state.read.pcNext;
            break;
        default: assert(!"Unexpected CUResSrc");
    }

    cpu.writebackStage.state.write.regWrite = state.read.execParams.regWrite;
    cpu.writebackStage.state.write.regAddr = state.read.regAddr;
}

void WritebackStage::Tick(CPU &cpu)
{}

} // namespace cpu
