#include "cpu_env.h"

namespace Sim {

CPUEnv::CPUEnv(void *mem, u32_t memSize, u32_t tvec)
{
    assert(mem && (memSize % sizeof(u32_t) == 0) && "Invalid memory");

    cpu.mmu.memory.resize(memSize / sizeof(u32_t));
    for (u32_t i = 0; i < memSize / sizeof(u32_t); ++i) {
        cpu.mmu.memory[i] = *(u32_t *)((u8_t *)mem + i * sizeof(u32_t));
    }

    u32_t mov00 = 0x00002023;

    (&cpu.mmu.memory[cpu.tvec / sizeof(u32_t)])[0] = mov00;
    (&cpu.mmu.memory[cpu.tvec / sizeof(u32_t)])[1] = mov00;
    (&cpu.mmu.memory[cpu.tvec / sizeof(u32_t)])[2] = mov00;
    (&cpu.mmu.memory[cpu.tvec / sizeof(u32_t)])[3] = mov00;
    cpu.shutdown = false;
}

void CPUEnv::Execute(u32_t pc)
{
    cpu.fetchStage.state.read.pc = pc;
    cpu.Execute();
}

} // namespace Sim
