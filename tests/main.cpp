#include <iostream>
#include "cpu.h"

#include <gtest/gtest.h>

#include "cpu.h"
#include "Emulator.h" // Assuming Emulator is your bus/memory system


class CPUTest : public ::testing::Test {
protected:
    CPU cpu;
    void SetUp() override {
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