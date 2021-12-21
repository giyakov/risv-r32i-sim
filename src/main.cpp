#include "cpu.h"
#include "types.h"

#include <cassert>
#include <bit>
#include <cstring>
#include <iostream>

struct CPUEnv final {
public:
    Sim::CPU cpu = {};

    static constexpr u32_t TVEC_HANDLER_SIZE = 16;

    CPUEnv(void *mem, u32_t memSize = 4096, u32_t tvec = 4096 - TVEC_HANDLER_SIZE)
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
    void Execute(u32_t pc);
};

void CPUEnv::Execute(u32_t pc)
{
    cpu.fetchStage.state.read.pc = pc;
    cpu.Execute();
}

void test_0()
{
    auto memory = std::vector<u32_t>(4096, 0);

    u32_t const code[] = {
        0x00100073U // ebreak
    };
    std::memcpy(memory.data() + 1024 / sizeof(u32_t), code, sizeof(code));

    auto env = CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - CPUEnv::TVEC_HANDLER_SIZE);

    std::cout << "**************** test_0\n";
    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024);
}

void test_1()
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
    auto env = CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - CPUEnv::TVEC_HANDLER_SIZE);

    env.cpu.mmu.memory[8] = 0x21323424;
    env.cpu.mmu.memory[9] = 0xdeadbabe;

    std::cout << "**************** test_1\n";
    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024 + 4 * sizeof(u32_t));

    assert(env.cpu.decodeStage.regfile.gpr[12] == 0xdeadbabe - 0x21323424);
    assert(env.cpu.mmu.memory[10] == 0xdeadbabe - 0x21323424);
}

void test_2()
{
    auto memory = std::vector<u32_t>(4096, 0);
    u32_t const code[] = {
        0x0080056fU, // jal a0, lab1
        0x07b00593U, // li a1, 123
        0x14100613U, // li a2, 321
        0x00100073U  // ebreak
    };
    std::memcpy(memory.data() + 1024 / sizeof(u32_t), code, sizeof(code));
    auto env = CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - CPUEnv::TVEC_HANDLER_SIZE);

    std::cout << "**************** test_2\n";
    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024 + 4 * 3);

    assert(env.cpu.decodeStage.regfile.gpr[10] == 1024 + 4);
    assert(env.cpu.decodeStage.regfile.gpr[11] == 0);
    assert(env.cpu.decodeStage.regfile.gpr[12] == 321);
}

void test_3()
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
    auto env = CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - CPUEnv::TVEC_HANDLER_SIZE);

    std::cout << "**************** test_3\n";
    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024 + 4 * 2);

    assert(env.cpu.decodeStage.regfile.gpr[1] == 1024 + 4 * 2);
    assert(env.cpu.decodeStage.regfile.gpr[2] == 1024);
    assert(env.cpu.decodeStage.regfile.gpr[10] == 0);
}

void test_4()
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
    auto env = CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - CPUEnv::TVEC_HANDLER_SIZE);

    std::cout << "**************** test_4\n";
    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024 + 4 * 2);

    assert(env.cpu.decodeStage.regfile.gpr[1] == 1024 + 4 * 2);
    assert(env.cpu.decodeStage.regfile.gpr[2] == 1024);
    assert(env.cpu.decodeStage.regfile.gpr[10] == 6);
}

void test_5()
{
    auto memory = std::vector<u32_t>(4096, 0);
    /* multiplication via loop, recursive fact */
    u32_t const code[] = {
        0x40000113U, 0x0c4000efU, 0x00100073U, 0x00000013U, 0xfd010113U, 0x02812623U, 0x03010413U, 0xfca42e23U,
        0xfcb42c23U, 0xfe042623U, 0xfe042423U, 0x0200006fU, 0xfec42703U, 0xfd842783U, 0x00f707b3U, 0xfef42623U,
        0xfe842783U, 0x00178793U, 0xfef42423U, 0xfe842703U, 0xfdc42783U, 0xfcf74ee3U, 0xfec42783U, 0x00078513U,
        0x02c12403U, 0x03010113U, 0x00008067U, 0xfe010113U, 0x00112e23U, 0x00812c23U, 0x02010413U, 0xfea42623U,
        0xfec42783U, 0x02078663U, 0xfec42783U, 0xfff78793U, 0x00078513U, 0xfd9ff0efU, 0x00050793U, 0x00078593U,
        0xfec42503U, 0xf6dff0efU, 0x00050793U, 0x0080006fU, 0x00100793U, 0x00078513U, 0x01c12083U, 0x01812403U,
        0x02010113U, 0x00008067U, 0xff010113U, 0x00112623U, 0x00812423U, 0x01010413U, 0x00500513U, 0xf91ff0efU,
        0x00050793U, 0x00078513U, 0x00c12083U, 0x00812403U, 0x01010113U, 0x00008067U
    };

    std::memcpy(memory.data() + 1024 / sizeof(u32_t), code, sizeof(code));
    auto env = CPUEnv(memory.data(), std::size(memory) * sizeof(u32_t),
        std::size(memory) * sizeof(u32_t) - CPUEnv::TVEC_HANDLER_SIZE);

    std::cout << "**************** test_5\n";
    env.Execute(1024);
    assert(env.cpu.huModule.exceptionPC == 1024 + 4 * 2);

    assert(env.cpu.decodeStage.regfile.gpr[1] == 1024 + 4 * 2);
    assert(env.cpu.decodeStage.regfile.gpr[2] == 1024);
    assert(env.cpu.decodeStage.regfile.gpr[10] == 120);
}

void test()
{
    test_0();
    test_1();
    test_2();
    test_3();
    test_4();
    test_5();
}

int main()
{
    test();
    return 0;
}
