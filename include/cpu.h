#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <variant>
#include <fstream>
#include <array>


class Emulator; // forward declare to avoid circular definition, need to to link read and write

class CPUTest;

class CPU
{
	friend class CPUTest;
public:

	CPU();
	void ConnectCPUToBus(Emulator* emu);

	void Reset();
	void Clock();


	enum Interrupt
	{
		VBLANK = 1,
		STAT = 1 << 1,
		TIMER = 1 << 2,
		SERIAL = 1 << 3,
		JOYPAD = 1 << 4
	};

	// represents what bit they were at
	// done before i realised i could have done it like Interrupt
	enum Flag 
	{
		C = 4,
		H = 5,
		N = 6,
		Z = 7
	};

	union Register
	{
		uint16_t reg;
		struct
		{
			uint8_t lo;
			uint8_t hi;
		};
	};

	enum AddressingMode {
		IMPL,
		IMM8,
		IMM16,
		REG8,
		REG16,
		IND,				// Indirect
		IND_IMM8,
		IND_IMM16,
		IND_REG8,
		COND
	};

	
	enum class CondType {
		NONE,
		Z, NZ,
		C, NC
	};

	 enum class RegType {
		NONE,
		A, B, C, D, E, H, L,
		AF, BC, DE, HL, SP,
		HLI, HLD
	};


	struct Operand {
		AddressingMode mode;
		RegType reg;
		CondType cond;
		int meta; // rst's return address or bit index or anything else encoded into the instruction
	};


	struct Instruction
	{
		std::string name = ""; // For disassembly
		uint8_t cycles = 0;
		void (CPU::* execute)() = nullptr;
		
		Operand operand1;
		Operand operand2;

	};

	Register AF;
	Register BC;
	Register DE;
	Register HL;

	uint16_t PC;
	uint16_t SP;


	bool halted = false;
	bool int_master_enabled = false;
	bool ime_enabling = false;

	uint8_t int_enable = 0;
	uint8_t int_flag = 0;

	Instruction InstructionByOpcode(uint8_t opcode);
	uint8_t m_Cycles = 0;

	void SetFlag(Flag flag, uint8_t value);
	uint8_t GetFlag(Flag flag);

	bool CheckInterrupt(Interrupt interupt_type, uint16_t address);
	void RequestInterrupt(Interrupt interrupt_type);

private:
	Emulator* emu;
	
	std::ofstream debug_file;

	Instruction HandleCBInstruction(uint8_t opcode);

	std::array<Instruction, 256> m_JumpTable;
	std::array<Instruction, 256> m_CBPrefixJumpTable;




	void cpu_push(uint8_t byte);
	uint8_t cpu_pop();

	void cpu_push16(uint16_t byte);
	uint16_t cpu_pop16();

public:
	// Instructions 
	void NOP();
	void HALT();
	void LD();
	void LD_HL_SP();
	void INC();
	void DEC();
	void ADD();
	void ADD_SP();
	void SUB();
	void ADC();
	void SBC();
	void AND();
	void XOR();
	void OR();
	void CP();
	void RET();
	void PUSH();
	void POP();
	void JP();
	void JR();
	void CALL();
	void RLCA();
	void RLA();
	void RRCA();
	void RRA();
	void SCF();
	void CPL();
	void CCF();
	void STOP();
	void RETI();
	void DI();
	void EI();
	void RST();
	void DAA();

	// CB instructions
	void RLC();
	void RRC();
	void RL();
	void RR();
	void SLA();
	void SRA();
	void SWAP();
	void SRL();
	void BIT();
	void SET();
	void RES();
	

	Instruction m_CurrentInstruction;

private:

	inline bool Is16Bit(const Operand& operand) {
		return operand.mode == REG16 ||
			   operand.mode == IMM16			   ;

	}


	uint16_t fetch(const Operand& operand);
	// fetching helper functions
	uint16_t getRegisterValue(RegType type);

	// might do function overloading but seems fine for now
	void writeOperand8(Operand& op, uint8_t operand);
	void writeOperand16(Operand& op, uint16_t operand);

	void writeRegister8(RegType reg, uint8_t value);
	void writeRegister16(RegType reg, uint16_t value);

	bool checkCond(CondType cond);


	
	// https://gbdev.io/pandocs/CPU_Instruction_Set.html
	static Operand DecodeReg8(uint8_t bits);
	static Operand DecodeReg16(uint8_t bits);
	static Operand DecodeReg16STK(uint8_t bits);
	static Operand DecodeReg16MEM(uint8_t bits);
	static Operand DecodeCond(uint8_t bits);


};

std::string_view CondTypeToString(const CPU::CondType cond);
std::string_view RegTypeToString(const CPU::RegType reg);
