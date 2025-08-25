#include <iostream>
#include "cpu.h"

#include <gtest/gtest.h>

#include "emulator.h" // Assuming Emulator is your bus/memory system


class CPUTest : public ::testing::Test {
protected:
    CPU cpu;
    Emulator emu;
    void SetUp() override {
       cpu.ConnectCPUToBus(&emu);
    }

    void TearDown() override {
        // Clean up if necessary
    }
};

TEST_F(CPUTest, NOPDoesNothing) {
    cpu.PC = 0x100;
    cpu.NOP();
    EXPECT_EQ(cpu.PC, 0x100);
}

TEST_F(CPUTest, INC_B_SetsFlagsCorrectly)
{
    cpu.PC = 0x100;
    cpu.BC.hi = 0x0F;
    
    CPU::Instruction incInstruction = {"INC", 1, &CPU::INC, {CPU::REG8, CPU::RegType::B}};
    cpu.m_CurrentInstruction = incInstruction;
    (cpu.*cpu.m_CurrentInstruction.execute)();

    EXPECT_EQ(cpu.BC.hi, 0x10);
    EXPECT_FALSE(cpu.GetFlag(FLAG_Z));
    EXPECT_FALSE(cpu.GetFlag(FLAG_N));
    EXPECT_TRUE(cpu.GetFlag(FLAG_H));

}

TEST_F(CPUTest, INC_B_ToZeroSetsZAndHFlags)
{
    cpu.PC = 0x100;
    cpu.BC.hi = 0xFF;  // 0xFF + 1 = 0x00 → Z and H set

    CPU::Instruction incInstruction = {"INC", 1, &CPU::INC, {CPU::REG8, CPU::RegType::B}};
    cpu.m_CurrentInstruction = incInstruction;
    (cpu.*cpu.m_CurrentInstruction.execute)();

    EXPECT_EQ(cpu.BC.hi, 0x00);
    EXPECT_TRUE(cpu.GetFlag(FLAG_Z));
    EXPECT_FALSE(cpu.GetFlag(FLAG_N));
    EXPECT_TRUE(cpu.GetFlag(FLAG_H));
}

TEST_F(CPUTest, DEC_B_SetsHalfCarryAndNFlags)
{
    cpu.PC = 0x100;
    cpu.BC.hi = 0x10;  // 0x10 - 1 = 0x0F → H and N set
    CPU::Instruction decInstruction = {"DEC", 1, &CPU::DEC, {CPU::REG8, CPU::RegType::B}};
    cpu.m_CurrentInstruction = decInstruction;
    (cpu.*cpu.m_CurrentInstruction.execute)();



    EXPECT_EQ(cpu.BC.hi, 0x0F);
    EXPECT_FALSE(cpu.GetFlag(FLAG_Z));
    EXPECT_TRUE(cpu.GetFlag(FLAG_N));
    EXPECT_TRUE(cpu.GetFlag(FLAG_H));
}

TEST_F(CPUTest, DEC_B_ToZeroSetsZNHFlags)
{
    cpu.PC = 0x100;
    cpu.BC.hi = 0x01;  // 0x01 - 1 = 0x00 → Z, N, H set

    CPU::Instruction decInstruction = {"DEC", 1, &CPU::DEC, {CPU::REG8, CPU::RegType::B}};
    cpu.m_CurrentInstruction = decInstruction;
    (cpu.*cpu.m_CurrentInstruction.execute)();


    EXPECT_EQ(cpu.BC.hi, 0x00);
    EXPECT_TRUE(cpu.GetFlag(FLAG_Z));
    EXPECT_TRUE(cpu.GetFlag(FLAG_N));
    EXPECT_FALSE(cpu.GetFlag(FLAG_H));
}

TEST_F(CPUTest, DEC_B_ClearsHalfCarryAndZSetsOnlyN)
{
    cpu.PC = 0x100;
    cpu.BC.hi = 0x22;  // 0x22 - 1 = 0x21 → N set, others clear

    CPU::Instruction decInstruction = {"DEC", 1, &CPU::DEC, {CPU::REG8, CPU::RegType::B}};
    cpu.m_CurrentInstruction = decInstruction;
    (cpu.*cpu.m_CurrentInstruction.execute)();

    EXPECT_EQ(cpu.BC.hi, 0x21);
    EXPECT_FALSE(cpu.GetFlag(FLAG_Z));
    EXPECT_TRUE(cpu.GetFlag(FLAG_N));
    EXPECT_FALSE(cpu.GetFlag(FLAG_H));
}

// TEST_F(CPUTest, LD_D_HL_IND)
// {
//     cpu.PC = 0x100;
//     cpu.BC.hi = 0XC7;  // 0x22 - 1 = 0x21 → N set, others clear

//     CPU::Instruction decInstruction = {"LD", 2, &CPU::LD, {CPU::REG8, CPU::RegType::D}, {CPU::IND, CPU::RegType::HL}};
//     cpu.m_CurrentInstruction = decInstruction;
//     (cpu.*cpu.m_CurrentInstruction.execute)();

//     EXPECT_EQ(cpu.PC, 0x100);
// }

TEST_F(CPUTest, DAA_AdditionTest) {
    struct DAA_TestCase {
        uint8_t A;
        bool N;
        bool H;
        bool C;
        uint8_t expectedA;
        bool expectedZ;
        bool expectedC;
    };

    std::vector<DAA_TestCase> tests = {
        {0x9A, false, false, false, 0x00, true, true},
        {0x15, false, false, false, 0x15, false, false},
        {0x9A, true, false, true,  0x3A, false, true},
        {0x66, false, false, false, 0x66, false, false},
        {0x6B, false, false, false, 0x71, false, false},
    };

    for (const auto& test : tests) {
        cpu.AF.hi = test.A;
        cpu.SetFlag(FLAG_N, test.N);
        cpu.SetFlag(FLAG_H, test.H);
        cpu.SetFlag(FLAG_C, test.C);

        cpu.DAA();

        EXPECT_EQ(cpu.AF.hi, test.expectedA) << "A failed for input " << std::hex << +test.A;
        EXPECT_EQ(cpu.GetFlag(FLAG_Z), test.expectedZ) << "Z flag failed for input " << std::hex << +test.A;
        EXPECT_EQ(cpu.GetFlag(FLAG_C), test.expectedC) << "C flag failed for input " << std::hex << +test.A;
        EXPECT_FALSE(cpu.GetFlag(FLAG_H)) << "H flag should always be cleared after DAA";
    }
}