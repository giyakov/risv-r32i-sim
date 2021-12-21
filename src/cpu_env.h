#ifndef SIM_CPU_ENV_H
#define SIM_CPU_ENV_H

#include <types.h>
#include <cpu.h>

namespace Sim {

struct CPUEnv final {
public:
    Sim::CPU cpu = {};
    static constexpr u32_t TVEC_HANDLER_SIZE = 16;

    CPUEnv(void *mem, u32_t memSize = 4096, u32_t tvec = 4096 - TVEC_HANDLER_SIZE);
    void Execute(u32_t pc);
};

} // namespace Sim

#endif // SIM_CPU_ENV_H
