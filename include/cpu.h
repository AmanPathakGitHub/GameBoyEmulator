#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <variant>

class Emulator; // forward declare to avoid circular definition, need to to link read and write

#define FLAG_C 4
#define FLAG_H 5
#define FLAG_N 6
#define FLAG_Z 7


class CPU
{
public:
	void ConnectCPUToBus(Emulator* emu);

	void Reset();
	void Clock();

	bool halted = false;

	union Register
	{
		uint16_t reg;
		struct
		{
			uint8_t lo;
			uint8_t hi;
		};
	};

	Register AF;
	Register BC;
	Register DE;
	Register HL;

	uint16_t PC;
	uint16_t SP;
private:

	Emulator* emu;
	uint8_t m_Cycles = 0;
	

	void SetFlag(uint8_t flag, uint8_t value);

	// Instructions 
	void NOP();
	void HALT();
	void LD();
	void INC();
	void DEC();
	void ADD();
	void SUB();



	struct Instruction
	{

		// if the operand is a register we will give it a reference to the actual value
		// bool is for condition
		using Operand = std::variant<uint8_t, uint16_t, bool, uint8_t*, uint16_t*>;

		enum OperandType {
			NONE,
			R8,
			R16,
			R16STK,
			R16MEM,			// this will be used for memory addresses aswell (a16)
			COND,
			TGT3,
			IMM8,
			IMM16
		};
	
		std::string name; // for disassembly
		uint8_t cycles;
		void (CPU::* execute)() = nullptr;

		OperandType operandType1 = NONE;
		Operand operand1;

		OperandType operandType2 = NONE;
		Operand operand2;

	};

	uint16_t fetch(Instruction::Operand operand=(uint8_t)0, Instruction::OperandType type=Instruction::IMM8);

	Instruction m_CurrentInstruction;

	
	Instruction InstructionByOpcode(uint8_t opcode);
	uint8_t* DecodeReg(uint8_t opcodeBits, Instruction::OperandType regType);

};