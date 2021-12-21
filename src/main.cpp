#include "cpu_env.h"

#include <cassert>
#include <cstring>

void Test0()
{
    auto memory = std::vector<u32_t>(4096, 0);

    u32_t const code[] = {
        0x00100073U // ebreak
    };
    std::memcpy(memory.data() + 1024 / sizeof(u32_t), code, sizeof(code));

    auto env = Sim::CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - Sim::CPUEnv::TVEC_HANDLER_SIZE);

    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024);
}

void Test1()
{
    auto memory = std::vector<u32_t>(4096, 0);
    u32_t const code[] = {
        0x02002503U, // lw a0, 32(zero)
        0x02402583U, // lw a1, 36(zero)
        0x40a58633U, // sub a2, a1, a0
        0x02c02423U, // sw a2, 40(zero)
        0x00100073U  // ebreak
    };
    std::memcpy(memory.data() + 1024 / sizeof(u32_t), code, sizeof(code));
    auto env = Sim::CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - Sim::CPUEnv::TVEC_HANDLER_SIZE);

    env.cpu.mmu.memory[8] = 0x21323424;
    env.cpu.mmu.memory[9] = 0xdeadbabe;

    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024 + 4 * sizeof(u32_t));

    assert(env.cpu.decodeStage.regfile.gpr[12] == 0xdeadbabe - 0x21323424);
    assert(env.cpu.mmu.memory[10] == 0xdeadbabe - 0x21323424);
}

void Test2()
{
    auto memory = std::vector<u32_t>(4096, 0);
    u32_t const code[] = {
        0x0080056fU, // jal a0, lab1
        0x07b00593U, // li a1, 123
        0x14100613U, // li a2, 321
        0x00100073U  // ebreak
    };
    std::memcpy(memory.data() + 1024 / sizeof(u32_t), code, sizeof(code));
    auto env = Sim::CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - Sim::CPUEnv::TVEC_HANDLER_SIZE);

    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024 + 4 * 3);

    assert(env.cpu.decodeStage.regfile.gpr[10] == 1024 + 4);
    assert(env.cpu.decodeStage.regfile.gpr[11] == 0);
    assert(env.cpu.decodeStage.regfile.gpr[12] == 321);
}

void Test3()
{
    auto memory = std::vector<u32_t>(4096, 0);
    u32_t const code[] = {
        0x40000113U, // li sp, 1024 (start)
        0x00c000efU, // jal ra, main
        0x00100073U, // ebreak
        0x00000013U, // nop
        0xff010113U, // addi sp,sp,-16 (main)
        0x00812623U, // sw s0,12(sp)
        0x01010413U, // addi s0,sp,16
        0x00000793U, // li a5,0
        0x00078513U, // mv a0,a5
        0x00c12403U, // lw s0,12(sp)
        0x01010113U, // addi sp,sp,16
        0x00008067U  // jr ra
    };
    std::memcpy(memory.data() + 1024 / sizeof(u32_t), code, sizeof(code));
    auto env = Sim::CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - Sim::CPUEnv::TVEC_HANDLER_SIZE);

    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024 + 4 * 2);

    assert(env.cpu.decodeStage.regfile.gpr[1] == 1024 + 4 * 2);
    assert(env.cpu.decodeStage.regfile.gpr[2] == 1024);
    assert(env.cpu.decodeStage.regfile.gpr[10] == 0);
}

void Test4()
{
    auto memory = std::vector<u32_t>(4096, 0);
    u32_t const code[] = {
        0x40000113U, // li sp, 1024 (start)
        0x00c000efU, // jal ra, main
        0x00100073U, // ebreak
        0x00000013U, // nop
        0xfe010113U, // addi sp,sp,-32 (main)
        0x00812e23U, // sw s0,28(sp)
        0x02010413U, // addi s0,sp,32
        0xfe042623U, // sw zero,-20(s0)
        0xfe042423U, // sw zero,-24(s0)
        0x01c0006fU, // j .L3
        0xfec42783U, // lw a5,-20(s0) (.L4)
        0x00278793U, // addi a5,a5,2
        0xfef42623U, // sw a5,-20(s0)
        0xfe842783U, // lw a5,-24(s0)
        0x00178793U, // addi a5,a5,1
        0xfef42423U, // sw a5,-24(s0)
        0xfe842703U, // lw a4,-24(s0) (.L3)
        0x00200793U, // li a5,2
        0xfee7d0e3U, // ble a4,a5,.L4
        0xfec42783U, // lw a5,-20(s0)
        0x00078513U, // mv a0,a5
        0x01c12403U, // lw s0,28(sp)
        0x02010113U, // addi sp,sp,32
        0x00008067U  // jr ra
    };
    std::memcpy(memory.data() + 1024 / sizeof(u32_t), code, sizeof(code));
    auto env = Sim::CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - Sim::CPUEnv::TVEC_HANDLER_SIZE);

    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024 + 4 * 2);

    assert(env.cpu.decodeStage.regfile.gpr[1] == 1024 + 4 * 2);
    assert(env.cpu.decodeStage.regfile.gpr[2] == 1024);
    assert(env.cpu.decodeStage.regfile.gpr[10] == 6);
}

int main()
{
    Test0();
    Test1();
    Test2();
    Test3();
    Test4();

    return 0;
}
