#ifndef SIM_TICK_MODULE_H
#define SIM_TICK_MODULE_H

namespace Sim {

struct CPU;

struct TickModule {
    virtual void Tick(CPU &cpu) = 0;
};

template<typename StateType>
struct TickState {
    StateType read = {};
    StateType write = {};

    void Tick() {
        read = write;
    }
};

} // namespace Sim

#endif // SIM_TICK_MODULE_H
