#include "cpu.h"

#include <iostream>

#include "emulator.h"


void CPU::ConnectCPUToBus(Emulator* emu)
{
	this->emu = emu;
}

void CPU::Reset()
{
	AF.reg = 0x01B0;
	BC.reg = 0x0013;
	DE.reg = 0x00D8;
	HL.reg = 0x014D;
	SP = 0xFFFE;
	PC = 0x100;

	// boot rom would be loaded into memory here
	// but apparently thats illegal to have so we just skip it
}


void CPU::Clock()
{
	if (m_Cycles == 0)
	{
		// execute instruction and update cycle count to tell it to wait

		uint8_t opcode = emu->read(PC++);
		
		m_CurrentInstruction = CPU::InstructionByOpcode(opcode);
		
		m_Cycles = m_CurrentInstruction.cycles;

		// might change to std::function
		(this->*m_CurrentInstruction.execute)();
	}

	m_Cycles--;
}


void CPU::SetFlag(uint8_t flag, uint8_t value)
{
	if (value == 1)
	{
		AF.lo |= (1 << flag);
	}
	else
	{
		AF.lo &= ~(1 << flag);
	}
}


// // gets the value at the operand, this function is mostly for register and R16MEM but i generalised for all types
// uint16_t CPU::fetch(Instruction::Operand operand, Instruction::OperandType type)
// {
// 	switch (type)
// 	{
// 	case CPU::Instruction::NONE:

// 		break;
// 	case CPU::Instruction::R8:
// 	{
// 		uint8_t* reg_ptr = std::get<uint8_t*>(operand);
// 		return *reg_ptr;
// 	}
		
// 	case CPU::Instruction::R16:
// 	{
// 		uint16_t* reg_ptr = (uint16_t*)std::get<uint8_t*>(operand);
// 		return *reg_ptr;
// 	}
// 	case CPU::Instruction::R16STK:
// 		break;
// 	case CPU::Instruction::R16MEM:
// 	{
// 		uint16_t address = std::get<uint16_t>(operand);
// 		return emu->read(address);
// 	}

// 	case CPU::Instruction::COND:
// 		break;
// 	case CPU::Instruction::TGT3:
// 		break;

// 	case CPU::Instruction::IMM8:
// 		return std::get<uint8_t>(operand);
	
// 	case CPU::Instruction::IMM16:
// 		return std::get<uint16_t>(operand);
// 	}

// 	return (uint8_t)0;
// }

uint16_t CPU::fetch(const Operand& op)
{
	if (auto v = std::get_if<IMM8>(&op))
		return (*v).value;
	else if (auto v = std::get_if<IMM16>(&op))
		return (*v).value;
	else if (auto v = std::get_if<REG8>(&op))
		return *((*v).reg_ptr);
	else if (auto v = std::get_if<REG16>(&op))
		return *((*v).reg_ptr);
	else if (auto v = std::get_if<MEM16>(&op))
		return emu->read16((*v).address); // IDK IF IT SHOULD BE READ16 OR JUST READ
	else if (auto v = std::get_if<std::monostate>(&op))
		std::cout << "ERROR FETCHED WHEN OPERAND IS MONOSTATE" << std::endl;
	else
		std::cout << "ERROR UNHANDLED OPERAND TYPE" << std::endl;

	return -1;

}

// writes 8 bits to the  (doesn't include immediates)
void CPU::writeOperand8(Operand& op, uint8_t value)
{
	if(auto v = std::get_if<REG8>(&op))
		*((*v).reg_ptr) = value;
	else if (auto v = std::get_if<MEM16>(&op))
		emu->write((*v).address, value);
	else
		std::cout << "ERROR WRITE OPERAND 8" << std::endl;
}


void CPU::writeOperand16(Operand& op, uint16_t value)
{
	if(auto v = std::get_if<REG16>(&op))
		*((*v).reg_ptr) = value;
	else if (auto v = std::get_if<MEM16>(&op))
		emu->write16((*v).address, value);
	else
		std::cout << "ERROR WRITE OPERAND 16" << std::endl;
}

void CPU::NOP()
{
	// do nothing
}

void CPU::HALT()
{
	halted = true;
}

void CPU::LD() 
{
	uint16_t value = fetch(m_CurrentInstruction.operand2);

	if(Instruction::Is16Bit(m_CurrentInstruction.operand2))
		writeOperand16(m_CurrentInstruction.operand1, value);
	else
		writeOperand8(m_CurrentInstruction.operand1, (uint8_t)value);
}

void CPU::INC()
{
	uint16_t value = fetch(m_CurrentInstruction.operand1);
	value += 1;

	if(Instruction::Is16Bit(m_CurrentInstruction.operand1))
		writeOperand16(m_CurrentInstruction.operand1, value);
	else
		writeOperand8(m_CurrentInstruction.operand1, (uint8_t)value);
}

void CPU::DEC()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	value -= 1;

	if(Instruction::Is16Bit(m_CurrentInstruction.operand1))
		writeOperand16(m_CurrentInstruction.operand1, value);
	else
		writeOperand8(m_CurrentInstruction.operand1, (uint8_t)value);}


CPU::Instruction CPU::InstructionByOpcode(uint8_t opcode) 
{
	switch(opcode)
	 {
	case 0x00:
		return {"NOP", 1, &CPU::NOP};
	case 0x08:
	{
		// LD (imm16), SP
		Operand address = MEM16 {emu->read16(PC)};
		PC += 2;
		return {"LD", 5, &CPU::LD, address, REG16 {&SP}};
	}
	case 0x76:
 		return { "HALT", 1, &CPU::HALT };
	
	default:
		if ((opcode & 0b11000000) == 0b01000000) // LD r8, r8
		{

			Operand dest = DecodeReg8((opcode & 0b00111000) >> 3); // possibly a uint16_t address pointing somewhere in gameboy memory (HL) 
			Operand src = DecodeReg8(opcode & 0b00000111);
			
			uint8_t cycles = std::holds_alternative<MEM16>(dest) ||
							 std::holds_alternative<MEM16>(src) ? 2 : 1;

			return { "LD", cycles, &CPU::LD, dest, src };

		}
		else if ((opcode & 0b11001111) == 0b00000001) // LD r16, imm16
		{
			Operand dest = DecodeReg16((opcode & 0b00110000) >> 4);
			Operand src = IMM16 {emu->read16(PC)};
			PC += 2; 
			return {"LD", 3, &CPU::LD, dest, src};
		}
		else if ((opcode & 0b11000111) == 0b00000110) // LD r8, imm8
		{
			Operand dest = DecodeReg8((opcode & 0b00111000) >> 3);
			Operand src = IMM8 {emu->read(PC++)};
			return { "LD", 2, &CPU::LD, dest, src };
		}

		else if ((opcode & 0b11001111) == 0b00000010) // LD r16mem, a
		{
			Operand dest = DecodeReg16MEM((opcode & 0b00110000) >> 4);
			return { "LD", 2, &CPU::LD, dest, REG8 {&AF.hi}};
		}
		else if ((opcode & 0b11001111) == 0b00001010) // LD a, r16mem
		{
			return {"LD", 4, &CPU::LD, REG8 {&AF.hi}, DecodeReg16MEM((opcode & 0b00110000) >> 4)};
		}
		else if ((opcode & 0b11000111) == 0b00000100) // INC r8
		{
			return {"INC", 1, &CPU::INC, DecodeReg8((opcode & 0b00111000) >> 3)};
		}
		else if ((opcode & 0b11000111) == 0b00000101) // DEC r8
		{
			return {"DEC", 1, &CPU::DEC, DecodeReg8((opcode & 0b00111000) >> 3)};
		}
		else 
		{
			std::cout << "ERROR UNHANDLED OPCODE" << std::endl;
		}
	}

	return {};
}


CPU::Operand CPU::DecodeReg8(uint8_t bits)
{
	switch(bits)
	{
		case 0: return REG8 {&BC.hi};
		case 1: return REG8 {&BC.lo};
		case 2: return REG8 {&DE.hi};
		case 3: return REG8 {&DE.lo};
		case 4: return REG8 {&HL.hi};
		case 5: return REG8 {&HL.lo};
		case 6: return MEM16 {HL.reg};
		case 7: return REG8 {&AF.hi};
	}
	std::cout << "ERROR DECODED INCORRECTLY" << std::endl;
	return {};
}

CPU::Operand CPU::DecodeReg16(uint8_t bits)
{
	switch(bits)
	{
		case 0: return REG16 {&BC.reg};
		case 1: return REG16 {&DE.reg};
		case 2: return REG16 {&HL.reg};
		case 3: return REG16 {&SP};
	}
	std::cout << "ERROR DECODED INCORRECTLY" << std::endl;
	return {};
}

CPU::Operand CPU::DecodeReg16STK(uint8_t bits)
{
	switch(bits)
	{
		case 0: return REG16 {&BC.reg};
		case 1: return REG16 {&DE.reg};
		case 2: return REG16 {&HL.reg};
		case 3: return REG16 {&AF.reg};
	}
	std::cout << "ERROR DECODED INCORRECTLY" << std::endl;
	return {};
}

CPU::Operand CPU::DecodeReg16MEM(uint8_t bits)
{
	switch(bits)
	{
		case 0: return MEM16 {BC.reg};
		case 1: return MEM16 {DE.reg};
		case 2: return MEM16 {HL.reg++};
		case 3: return MEM16 {HL.reg--};
	}
	std::cout << "ERROR DECODED INCORRECTLY" << std::endl;
	return {};
}



// void CPU::LD()
// {

// 	uint16_t value = fetch(m_CurrentInstruction.operand2, m_CurrentInstruction.operandType2);
	

// 	if (m_CurrentInstruction.operandType1 == Instruction::OperandType::R8)
// 	{
// 		uint8_t* destRegister = std::get<uint8_t*>(m_CurrentInstruction.operand1);
// 		*destRegister = value;
// 	}
// 	else if (m_CurrentInstruction.operandType1 == Instruction::OperandType::R16)
// 	{
// 		uint16_t* destRegister = (uint16_t*)std::get<uint8_t*>(m_CurrentInstruction.operand1);
// 		*destRegister = value;
// 	}
// 	else if (m_CurrentInstruction.operandType1 == Instruction::OperandType::R16MEM)
// 	{
// 		uint16_t address = std::get<uint16_t>(m_CurrentInstruction.operand1);
		
// 		// LD (a16), SP
// 		if (m_CurrentInstruction.operandType2 == Instruction::OperandType::R16) emu->write16(address, value);
// 		else emu->write(address, (uint8_t)value);
// 	}


// }

// void CPU::INC()
// {
// 	uint8_t* reg = std::get<uint8_t*>(m_CurrentInstruction.operand1);
// 	(*reg)++;
// }

// void CPU::DEC()
// {
// 	uint8_t* reg = std::get<uint8_t*>(m_CurrentInstruction.operand1);
// 	(*reg)--;
// }

// void CPU::ADD()
// {
// 	uint8_t* reg = std::get<uint8_t*>(m_CurrentInstruction.operand1);
// 	uint8_t value = std::get<uint8_t>(m_CurrentInstruction.operand2);
// 	*reg += value;
// }
// void CPU::SUB()
// {
// 	uint8_t* reg = std::get<uint8_t*>(m_CurrentInstruction.operand1);
// 	uint8_t value = std::get<uint8_t>(m_CurrentInstruction.operand2);
// 	*reg -= value;
// }



// base on https://gbdev.io/pandocs/CPU_Instruction_Set.html 
// dynamically creating instructions by looking at their bits
// CPU::Instruction CPU::InstructionByOpcode(uint8_t opcode)
// {
// 	using OpType = CPU::Instruction::OperandType;

// 	// switch case statement for unrelated opcodes
// 	switch (opcode) {
// 	case 0x00:
// 		return { "NOP", 1, &CPU::NOP};
// 	case 0x76:
// 		return { "HALT", 1, &CPU::HALT };

// 	default:
// 		// grouped together stuff
// 		if ((opcode & 0b11000000) == 0b01000000) // LD r8, r8
// 		{
// 			uint8_t regbits_dest = (opcode & 0b00111000) >> 3;
// 			uint8_t regbits_src = opcode & 0b00000111;

// 			Instruction::Operand dest; // possibly a uint16_t address pointing somewhere in gameboy memory (HL) 
// 			Instruction::Operand src;
// 			Instruction::OperandType dest_type = Instruction::OperandType::R8;
// 			Instruction::OperandType src_type = Instruction::OperandType::R8;
// 			uint8_t cycles = 1;

// 			// if bits are 6 then it is indirect addressing to HL
// 			// TODO probably can wrap this code in a function
// 			// might have to change Optype to R16MEM
// 			if (regbits_dest == 6)
// 			{
// 				dest = HL.reg;
// 				dest_type = Instruction::OperandType::R16MEM;
// 				cycles = 2;
// 			} 
// 			else
// 				dest = DecodeReg(regbits_dest, OpType::R8);

// 			if (regbits_src == 6)
// 			{
// 				src = HL.reg;
// 				src_type = Instruction::OperandType::R16MEM;
// 				cycles = 2;
// 			} 
// 			else
// 				src = DecodeReg(regbits_src, OpType::R8);

// 			return { "LD", cycles, &CPU::LD, dest_type, dest , src_type, src };

// 		}
// 		else if ((opcode & 0b11001111) == 0b00000001) // LD r16, imm16
// 		{
// 			uint8_t* reg = DecodeReg((opcode & 0b00110000) >> 4, Instruction::OperandType::R16);
// 			uint16_t value = emu->read16(PC);
// 			PC += 2;

// 			return { "LD", 3, &CPU::LD, Instruction::R16, reg, Instruction::IMM16, value};
// 		}
// 		else if ((opcode & 0b11000111) == 0b00000110) // LD r8, imm8
// 		{
// 			uint8_t* reg = DecodeReg((opcode & 0b00111000) >> 3, Instruction::OperandType::R8);
// 			uint8_t value = emu->read(PC++);

// 			return { "LD", 2, &CPU::LD, Instruction::R8, reg, Instruction::IMM8, value };
// 		}
// 		else if ((opcode & 0b11001111) == 0b00000010) // LD r16mem, a
// 		{
// 			uint8_t bits = (opcode & 0b00110000) >> 4;
// 			uint16_t* reg16 = (uint16_t*)DecodeReg(bits, Instruction::R16MEM);
// 			uint16_t address = *reg16;
			
// 			// this will be HL could replace reg16 with HL.reg might make it clearer
// 			if (bits == 2) (*reg16)++; // increment HL
// 			else if (bits == 3) (*reg16)--; // decrement HL

// 			return { "LD", 2, &CPU::LD, Instruction::R16MEM, address, Instruction::R8, &AF.hi };
// 		}
// 		else if ((opcode & 0b11001111) == 0b00001010) // LD a, r16mem
// 		{
// 			uint8_t bits = (opcode & 0b00110000) >> 4;
// 			uint16_t* reg16 = (uint16_t*)DecodeReg(bits, Instruction::R16MEM);
// 			uint16_t address = *reg16;
// 			// this will be HL could replace reg16 with HL.reg might make it clearer
// 			if (bits == 2) (*reg16)++; // increment HL
// 			else if (bits == 3) (*reg16)--; // decrement HL
// 			return { "LD", 4, &CPU::LD, Instruction::R8, &AF.hi, Instruction::R16MEM, address };
// 		}
// 		else if (opcode == 0b00001000) // LD (imm16), sp
// 		{
// 			uint16_t address = emu->read16(PC);
// 			PC += 2;
// 			return { "LD", 5, &CPU::LD, Instruction::R16MEM, address, Instruction::R16, (uint8_t*)(&SP)};
// 		}
// 		else if ((opcode & 0b11001111) == 0b00000011) // INC r16
// 		{
// 			uint8_t bits = (opcode & 0b00110000) >> 4;
// 			uint8_t* reg = DecodeReg(bits, Instruction::R16);

// 			return { "INC", 2, &CPU::INC, Instruction::R16, reg };
// 		}
// 		else if ((opcode & 0b11000111) == 0b00000100) // INC r8
// 		{
// 			uint8_t bits = (opcode & 0b00111000) >> 3;
// 			uint8_t* reg = DecodeReg(bits, Instruction::R8);
// 			return { "INC", 1, &CPU::INC, Instruction::R8, reg };
// 		}
// 		else if ((opcode & 0b11001111) == 0b00000011) // DEC r16
// 		{
// 			uint8_t bits = (opcode & 0b00110000) >> 4;
// 			uint8_t* reg = DecodeReg(bits, Instruction::R16);
// 			return { "DEC", 2, &CPU::DEC, Instruction::R16, reg };
// 		}
// 		else if ((opcode & 0b11000111) == 0b00000101) // DEC r8
// 		{
// 			uint8_t bits = (opcode & 0b00111000) >> 3;
// 			uint8_t* reg = DecodeReg(bits, Instruction::R8);
	
// 			return { "DEC", 1, &CPU::DEC, Instruction::R8, reg };
// 		}

// 		else
// 		{
// 			std::cout << "UNHANDLED OPCODE " << opcode << std::endl;
// 			__debugbreak();
// 		}
// 	}

// 	return {};
// }


// uint8_t* CPU::DecodeReg(uint8_t bits, CPU::Instruction::OperandType regType)
// {
// 	switch (regType)
// 	{
// 	case CPU::Instruction::R8:

// 		// if its 6 it is indirect and this function should not handle it
// 		// as the operand is not a register but an address in memory
// 		if (bits == 0) return &BC.hi;
// 		else if (bits == 1) return &BC.lo;
// 		else if (bits == 2) return &DE.hi;
// 		else if (bits == 3) return &DE.lo;
// 		else if (bits == 4) return &HL.hi;
// 		else if (bits == 5) return &HL.lo;
// 		//else if (bits == 6) return (uint8_t*)(&HL.reg); // could just return &HL.lo but want to make it 
// 		else if (bits == 7) return &AF.hi;

// 		break;
// 	case CPU::Instruction::R16:
// 		if (bits == 0) return (uint8_t*)(&BC.reg);
// 		else if (bits == 1) return (uint8_t*)(&DE.reg);
// 		else if (bits == 2) return (uint8_t*)(&HL.reg);
// 		else if (bits == 3) return (uint8_t*)(&SP);
		
// 		break;
// 	case CPU::Instruction::R16STK:
// 		break;
// 	case CPU::Instruction::R16MEM:
// 		if (bits == 0) return (uint8_t*)(&BC.reg);
// 		else if (bits == 1) return (uint8_t*)(&DE.reg);
// 		else if (bits == 2) return (uint8_t*)(&HL.reg);
// 		else if (bits == 3) return (uint8_t*)(&HL.reg);

// 		break;

// 	default:
// 		std::cout << "INVALID OPERAND TYPE" << std::endl;
// 		__debugbreak();
// 	}
// }