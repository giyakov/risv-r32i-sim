#ifndef SIM_CPU_H
#define SIM_CPU_H

#include <types.h>
#include <isa.h>
#include <vector>

namespace Sim {

enum class HURS : u8_t {
    REG, BP_MEM, BP_WB
};

enum class HUExceptionType : u8_t {
    NONE,
    BAD_OPCODE, UNALIGNED_ADDR, MMU_MISS, INT,
};

enum class HUExcecutionStage : u8_t {
    NONE,
    FETCH, DECODE, EXECUTE, MEMORY, WRITEBACK,
};

struct HUModule final : public TickModule {
public:
    u32_t exceptionPC = 0;
    HUExcecutionStage exceptionExecStage = HUExcecutionStage::NONE;
    HUExceptionType exceptionType = HUExceptionType::NONE;

    void Tick(CPU &cpu) override;
    void Raise(HUExcecutionStage stage, HUExceptionType type, u32_t pc);

    HURS GetRS(CPU& cpu, u8_t rsa);
};

struct MMU final {
public:
    HUExceptionType Load(CPU &cpu, u32_t a, u32_t *dst, CUMemOp memOp = CUMemOp::WORD);
    HUExceptionType Store(CPU &cpu, u32_t a, u32_t data, CUMemOp memOp = CUMemOp::WORD);

    std::vector<u32_t> memory = {};
};

struct FetchStage final : public TickModule {
public:
    struct State final {
        u32_t pc = 0;
    };
    TickState<State> state = {};

    void Tick(CPU &cpu) override;
};

struct DecodeStage final : public TickModule {
public:
    struct State final {
        Instruction inst = {};
        u32_t pc = 0;
        u32_t pcNext = 0;
        bool v = true;
    };
    TickState<State> state = {};

    struct Regfile final : TickModule {
        u32_t gpr[32] = {};
        void Tick(CPU &cpu) override;
    } regfile;

    void Tick(CPU &cpu) override;

    CUExecParams DecodeInstruction(Instruction inst);
    u32_t UnpackImmediate(Instruction inst, InstructionType iType);
};

struct ExecuteStage final : public TickModule {
public:
    struct State final {
        CUExecParams execParams = {};
        u32_t pc = 0;
        u32_t pcNext = 0;
        u32_t rs1v = 0;
        u32_t rs2v = 0;
        u32_t immExt = 0;
        u8_t rs1a = 0;
        u8_t rs2a = 0;
        u8_t rda = 0;
    };
    TickState<State> state = {};

    u32_t jumpBase = 0;
    bool pcR = false;

    void Tick(CPU &cpu) override;

    u32_t ALUOperator(CUALUOp op, u32_t rs1v, u32_t rs2v);
    bool CMPOperator(CUCmpOp op, u32_t rs1v, u32_t rs2v);
};

struct MemoryStage final : public TickModule {
public:
    struct State {
        CUExecParams execParams = {};
        u8_t regAddr = 0;
        u32_t pcNext = 0;
        u32_t pc = 0;
        u32_t memWdata = 0;
        u32_t aluRes = 0;
    };
    TickState<State> state = {};

    struct UOp {
        u32_t a = 0;
        u32_t d = 0;
        bool active = false;
    } delayedWrite;

    void Tick(CPU &cpu) override;
};

struct WritebackStage final : public TickModule {
public:
    struct State {
        bool regWrite = false;
        u8_t regAddr = 0;
        u32_t regWdata = 0;
    };
    TickState<State> state = {};

    void Tick(CPU &cpu) override;
};

struct CPU final {
public:
    MMU mmu = {};
    HUModule huModule = {};

    FetchStage fetchStage = {};
    DecodeStage decodeStage = {};
    ExecuteStage executeStage = {};
    MemoryStage memoryStage = {};
    WritebackStage writebackStage = {};

    bool shutdown = true;
    u32_t tvec = 0;

    void Tick();
    void Execute();
};

} // namespace Sim

#endif // SIM_CPU_H
