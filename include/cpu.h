#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <cstdint>


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

	// creating types to seperate the different operands from each other
	// needed because IMM16 and MEM16 are both uint16_t but two different things
	// if the operand is a register we will give it a reference to the actual value
	// bool is for condition
	struct IMM8 	{ uint8_t value; };
	struct IMM16 	{ uint16_t value; };
	struct REG8 	{ uint8_t* reg_ptr; };
	struct REG16  	{ uint16_t* reg_ptr; };
	struct MEM16    { uint16_t address; };
	struct COND		{ bool cond; }; 	// maybe should be uint8_t idk?

	using Operand = std::variant<std::monostate, IMM8, IMM16, REG8, REG16, MEM16, COND>;

	struct Instruction
	{
		std::string name; // for disassembly
		uint8_t cycles;
		void (CPU::* execute)() = nullptr;

		Operand operand1;
		Operand operand2;

		static bool Is16Bit(const Operand& operand) {
    		return std::holds_alternative<REG16>(operand) ||
           		   std::holds_alternative<MEM16>(operand) ||
           		   std::holds_alternative<IMM16>(operand);
		}

	};

	uint16_t fetch(const Operand& operand);

	Instruction m_CurrentInstruction;

	
	Instruction InstructionByOpcode(uint8_t opcode);
	
	// https://gbdev.io/pandocs/CPU_Instruction_Set.html
	Operand DecodeReg8(uint8_t bits);
	Operand DecodeReg16(uint8_t bits);
	Operand DecodeReg16STK(uint8_t bits);
	Operand DecodeReg16MEM(uint8_t bits);




	// might make it have the same name?
	void writeOperand8(Operand& op, uint8_t operand);
	void writeOperand16(Operand& op, uint16_t operand);


};