#include "cpu.h"

#include <iostream>

#include "emulator.h"
#include <sstream>
#include <format>

CPU::CPU()
{
	debug_file.open("log2.txt");
	
	for(int opcode = 0; opcode <= 0xFF; opcode++)
	{
		if (opcode == 0xCB) continue;
		m_JumpTable[opcode] = InstructionByOpcode(opcode);
	}

	for (int opcode = 0; opcode <= 0xFF; opcode++)
	{
		m_CBPrefixJumpTable[opcode] = HandleCBInstruction(opcode);
	}


	Reset();

}

void CPU::ConnectCPUToBus(Emulator* emu)
{
	this->emu = emu;
}


std::string_view RegTypeToString(const CPU::RegType reg)
{
	switch (reg)
	{
	case CPU::RegType::NONE: return "NONE";
	case CPU::RegType::A:    return "A";
	case CPU::RegType::B:    return "B";
	case CPU::RegType::C:    return "C";
	case CPU::RegType::D:    return "D";
	case CPU::RegType::E:    return "E";
	case CPU::RegType::H:    return "H";
	case CPU::RegType::L:    return "L";
	case CPU::RegType::AF:   return "AF";
	case CPU::RegType::BC:   return "BC";
	case CPU::RegType::DE:   return "DE";
	case CPU::RegType::HL:   return "HL";
	case CPU::RegType::SP:   return "SP";
	case CPU::RegType::HLI:  return "HLI";
	case CPU::RegType::HLD:  return "HLD";
	default: return "UNKNOWN";
	}
}

std::string_view CondTypeToString(const CPU::CondType cond)
{
	switch (cond)
	{
	case CPU::CondType::NONE: return "NONE";
	case CPU::CondType::Z:    return "Z";
	case CPU::CondType::NZ:   return "NZ";
	case CPU::CondType::C:    return "C";
	case CPU::CondType::NC:   return "NC";
	default: return "UNKNOWN";
	}
}

void WriteOp(CPU::Operand& op, std::stringstream& ss)
{
		switch(op.mode)
		{
			case CPU::AddressingMode::IMM8:
				ss << " imm8";  
				break;
			case CPU::AddressingMode::IMM16:
				ss << " imm16";
				break;
			
			case CPU::AddressingMode::REG16:
			case CPU::AddressingMode::REG8:
				ss << " " << RegTypeToString(op.reg);
				break;
			case CPU::AddressingMode::IND:
				ss << " (" << RegTypeToString(op.reg) << ")";
				break;
			case CPU::AddressingMode::IND_IMM8:
				ss << " (imm8)";
				break;
			case CPU::AddressingMode::IND_IMM16:
				ss << " (imm16)";
				break;
			case CPU::AddressingMode::COND:
				ss << " " << CondTypeToString(op.cond);

		}
}


std::string dissassembleInstr(CPU::Instruction ins, int opcode )
{
	std::stringstream ss;

	ss << "		" <<  std::hex << (int)opcode << " " << ins.name;
	
	WriteOp(ins.operand1, ss);
	WriteOp(ins.operand2, ss);

	return ss.str();
}



void CPU::Reset()
{
	
	AF.reg = 0x01B0;
	BC.reg = 0x0013;
	DE.reg = 0x00D8;
	HL.reg = 0x014D;
	SP = 0xFFFE;
	PC = 0x100;

	halted = false;


	int_master_enabled = false;
	
	ime_enabling = false;

	int_enable = 0;
	int_flag = 0;

	m_Cycles = 0;

}



void CPU::Clock()
{	

	// Wake up from HALT if any interrupt is pending (even if IME is off)
    if (halted && (int_enable & int_flag)) {
        halted = false;
        // But don't service the interrupt this frame unless IME is enabled
    }

    // If IME is enabled and not halted, service interrupts
    if (!halted && int_master_enabled) {
        if (CheckInterrupt(VBLANK, 0x40)) return;
        if (CheckInterrupt(STAT,    0x48)) return;
        if (CheckInterrupt(TIMER,  0x50)) return;
        if (CheckInterrupt(SERIAL, 0x58)) return;
        if (CheckInterrupt(JOYPAD, 0x60)) return;
    }

	if(!halted)
	{
		
		if (m_Cycles == 0)
		{
			
			if (ime_enabling) {
        		int_master_enabled = true;
        		ime_enabling = false;
  			}

			//if(PC == 0xC06C) __debugbreak();

			// debug_file << std::format("A:{:02X} F:{:02X} B:{:02X} C:{:02X} D:{:02X} E:{:02X} H:{:02X} L:{:02X} SP:{:04X} PC:{:04X} PCMEM:{:02X},{:02X},{:02X},{:02X}\n"
			// 	, AF.hi, AF.lo, BC.hi, BC.lo, DE.hi, DE.lo, HL.hi, HL.lo, SP, PC, emu->read(PC), emu->read(PC + 1), emu->read(PC + 2), emu->read(PC + 3));
			//std::vector<std::string> d = Application::dissassemble(*emu, PC, PC + 5); // this is wrong???
			uint8_t opcode = emu->read(PC++);			
			/* debug_file << d[0] <<
			  " Z: " << (int)GetFlag(Flag::Z) << " C: " << (int)GetFlag(Flag::C) << " N: " <<  (int)GetFlag(Flag::N) << " H: " <<  (int)GetFlag(Flag::H) 
			  << " AF: 0x" << std::hex << (int)AF.reg << " BC: 0x" << std::hex << (int)BC.reg << " DE: 0x" << std::hex << (int)DE.reg << " HL: 0x" << std::hex << (int)HL.reg 
			  << " SP: 0x" << std::hex << (int)SP 
			  << "\n";
			*/
			

			if (opcode == 0xCB)
			{
				uint8_t cbOpcode = emu->read(PC++);
				m_CurrentInstruction = m_CBPrefixJumpTable[cbOpcode];
			}
			else
				m_CurrentInstruction = m_JumpTable[opcode];
			
			

			
			m_Cycles = m_CurrentInstruction.cycles;

			// might change to std::function
			(this->*m_CurrentInstruction.execute)();
			
		}

		m_Cycles--;

		
	}

}


bool CPU::CheckInterrupt(CPU::Interrupt interupt_type, uint16_t address)
{
	if((int_enable & interupt_type) && (int_flag & interupt_type))
	{
		cpu_push16(PC);
		PC = address;

		int_master_enabled = false;
		int_flag &= ~interupt_type;
		halted = false;

		return true;
	}
	return false;
}

void CPU::RequestInterrupt(CPU::Interrupt interrupt_type)
{
	int_flag |= interrupt_type;
}

uint8_t CPU::GetFlag(Flag flag)
{
	return (AF.lo & (1 << flag)) != 0;
}


void CPU::SetFlag(Flag flag, uint8_t value)
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


void CPU::writeRegister8(RegType reg, uint8_t value)
{
	switch (reg) {
		case RegType::A:
			AF.hi = value;
			break;
		case RegType::B:
			BC.hi = value;
			break;
		case RegType::C:
			BC.lo = value;
			break;
		case RegType::D:
			DE.hi = value;
			break;
		case RegType::E:
			DE.lo = value;
			break;
		case RegType::H:
			HL.hi = value;
			break;
		case RegType::L:
			HL.lo = value;
			break;
		default:
			std::cout << "INVALID REG 8 WRITE OPERATION" << std::endl;
	}
}

void CPU::writeRegister16(RegType reg, uint16_t value)
{
	switch (reg)
	{
	case RegType::AF:
		AF.reg = value;
		break;
	case RegType::BC:
		BC.reg = value;
		break;
	case RegType::DE:
		DE.reg = value;
		break;
	case RegType::HL:
		HL.reg = value;
		break;
	case RegType::SP:
		SP = value;
		break;
	default:
		std::cout << "INVALID REG 16 WRITE OPERATION" << std::endl;
		break;
	}
}

void CPU::writeOperand8(Operand& op, uint8_t value)
{
	switch (op.mode)
	{
	case REG8:

		writeRegister8(op.reg, value);
		break;
	case IND: // pretty sure we can only indirectly write a single byte so this part isnt in writeoperand16


		emu->write(getRegisterValue(op.reg), value);

		if(op.reg == RegType::HLI) HL.reg++;
		else if (op.reg == RegType::HLD) HL.reg--;

		break;
	case IND_IMM8: // only used for LD (a8), A. see for more info
	{
		uint16_t address = 0xFF00 | emu->read(PC++);
		emu->write(address, value);
	}
		break;
	case IND_IMM16: // used for LD (a16), A
	{
		uint16_t address = emu->read16(PC);
		PC += 2;
		emu->write(address, value);
	}
		break;
	case IND_REG8:
	{
		uint16_t address = 0xFF00 | getRegisterValue(op.reg);
		emu->write(address, value);
	}
	break;
	default:
		std::cout << "INVALID WRITE 8" << std::endl;
		break;
	}
}

void CPU::writeOperand16(Operand& op, uint16_t value)
{
	switch (op.mode)
	{
	case REG16:
		writeRegister16(op.reg, value);
		break;
	case IND_IMM16: // used for LD (a16), SP
	{
		uint16_t address = emu->read16(PC);
		PC += 2;
		emu->write16(address, value);
	}
		break;
	default:
		std::cout << "INVALID WRITE 16" << std::endl;
		break;
	}
}


uint16_t CPU::getRegisterValue(RegType type)
{
	switch (type)
	{
	case RegType::A: return AF.hi;
	case RegType::B: return BC.hi;
	case RegType::C: return BC.lo;
	case RegType::D: return DE.hi;
	case RegType::E: return DE.lo;
	case RegType::H: return HL.hi;
	case RegType::L: return HL.lo;
	case RegType::AF: return AF.reg;
	case RegType::BC: return BC.reg;
	case RegType::DE: return DE.reg;
	case RegType::HLI:
	case RegType::HLD:
	case RegType::HL: return HL.reg;
	case RegType::SP: return SP;			
	default:
		std::cout << "INVALID REG TYPE" << std::endl;
		return -1;
	}
}

uint16_t CPU::fetch(const Operand& operand)
{
	switch (operand.mode)
	{
	case REG8:
		return getRegisterValue(operand.reg) & 0Xff;
	case REG16:
		return getRegisterValue(operand.reg);
	case IND:
	{
		uint8_t result = emu->read(getRegisterValue(operand.reg));

		if(operand.reg == RegType::HLI) HL.reg++;
		else if (operand.reg == RegType::HLD) HL.reg--;
		
		return result;
	}
	break;
	case IMM8:
		return emu->read(PC++);
	case IMM16:
	{
		uint16_t value = emu->read16(PC);
		PC += 2;
		return value;
	}
	case IND_IMM8:
	{
		uint16_t address = 0xFF00 | emu->read(PC++);
		return emu->read(address);
	}
	case IND_IMM16:
	{
		uint16_t address = emu->read16(PC);
		PC += 2;
		uint8_t value = emu->read(address);
		return value;
	}
	case IND_REG8:
	{
		uint16_t address = 0xFF00 | getRegisterValue(operand.reg);
		return emu->read(address);
	}
	default:
		std::cout << "INVALID ADDRESSING MODE" << std::endl;
		return -1;
	}
}

// FULL DESCENDING STACK
void CPU::cpu_push(uint8_t byte)
{
	emu->write(--SP, byte);
}

uint8_t CPU::cpu_pop()
{
	return emu->read(SP++);
}

void CPU::cpu_push16(uint16_t byte)
{
 	uint8_t hi = (byte >> 8) & 0xFF;
	uint8_t lo = byte & 0xFF;

	cpu_push(hi);
	cpu_push(lo);
}

uint16_t CPU::cpu_pop16()
{
	uint16_t lo = cpu_pop();
	uint16_t hi = cpu_pop();

	return (hi << 8) | lo;
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


	if(Is16Bit(m_CurrentInstruction.operand2))
		writeOperand16(m_CurrentInstruction.operand1, value);
	else
		writeOperand8(m_CurrentInstruction.operand1, (uint8_t)value);
}

void CPU::LD_HL_SP()
{
	// this is specific no need to use the abstracted functions

	int8_t value = fetch(m_CurrentInstruction.operand1) & 0xFF;
	HL.reg = SP + value;

	SetFlag(Flag::Z, 0);
	SetFlag(Flag::N, 0);
	SetFlag(Flag::H, ((SP & 0xF) + (value & 0xF)) > 0xF);
    SetFlag(Flag::C, ((SP & 0xFF) + (value & 0xFF)) > 0xFF);
}

void CPU::INC()
{
	uint16_t value = fetch(m_CurrentInstruction.operand1);

	if(Is16Bit(m_CurrentInstruction.operand1))
	{
		value++;
		writeOperand16(m_CurrentInstruction.operand1, value);

	}
	else
	{
		uint8_t result = value + 1;
		writeOperand8(m_CurrentInstruction.operand1, (uint8_t)result);

		SetFlag(Flag::Z, (uint8_t)result == 0);
		SetFlag(Flag::N, 0);
		SetFlag(Flag::H, (((uint8_t)value & 0x0F) + 1) > 0xF);
	}

}
void CPU::DEC()
{
	uint16_t value = fetch(m_CurrentInstruction.operand1);

	if(Is16Bit(m_CurrentInstruction.operand1))
	{
		value--;
		writeOperand16(m_CurrentInstruction.operand1, value);
	}
		
	else
	{
		uint8_t result = value - 1;
		writeOperand8(m_CurrentInstruction.operand1, (uint8_t)result);
		SetFlag(Flag::Z, (uint8_t)result == 0);
		SetFlag(Flag::N, 1);
		SetFlag(Flag::H, ((uint8_t)value & 0x0F) == 0);
	}

	
}

void CPU::ADD() 
{
	uint16_t value1 = fetch(m_CurrentInstruction.operand1);
	uint16_t value2 = fetch(m_CurrentInstruction.operand2);
	
	if(Is16Bit(m_CurrentInstruction.operand1))
	{
		uint32_t temp = (uint32_t)value1 + (uint32_t)value2;
		uint16_t result = (uint16_t)temp;
		SetFlag(Flag::C, temp > 0xFFFF);
		SetFlag(Flag::N, 0);
		SetFlag(Flag::H, ((value1 & 0xFFF) + (value2 & 0xFFF)) > 0xFFF);
		writeOperand16(m_CurrentInstruction.operand1, result);
	}
	else
	{
		uint16_t temp = value1 + value2;
		uint8_t result = (uint8_t)temp;
		SetFlag(Flag::C, temp > 0xFF);
		SetFlag(Flag::Z, result == 0);
		SetFlag(Flag::N, 0);
		SetFlag(Flag::H, ((value1 & 0xF) + (value2 & 0xF)) > 0xF);
		writeOperand8(m_CurrentInstruction.operand1, result);
	}
}

void CPU::ADD_SP()
{
	int8_t value = fetch(m_CurrentInstruction.operand1); // signed 8-bit
	uint16_t sp = SP;
	uint16_t result = sp + value;

	SetFlag(Flag::Z, 0); // Always cleared
	SetFlag(Flag::N, 0); // Always cleared

	SetFlag(Flag::H, ((sp & 0xF) + (value & 0xF)) > 0xF);
	SetFlag(Flag::C, ((sp & 0xFF) + (value & 0xFF)) > 0xFF);

	SP = result;
}

void CPU::ADC()
{
	uint16_t value1 = fetch(m_CurrentInstruction.operand1);
	uint16_t value2 = fetch(m_CurrentInstruction.operand2);
	uint8_t carry = GetFlag(Flag::C);
	
	if(Is16Bit(m_CurrentInstruction.operand1))
	{
		uint32_t temp = (uint32_t)value1 + (uint32_t)value2 + (uint32_t)carry;
		uint16_t result = temp & 0xFFFF;
		
		SetFlag(Flag::C, temp > 0xFFFF);
		SetFlag(Flag::N, 0);
		SetFlag(Flag::H, ((value1 & 0xFFF) + (value2 & 0xFFF) + carry) > 0xFFF);

		writeOperand16(m_CurrentInstruction.operand1, result);
	}
	else
	{
		uint16_t temp = value1 + value2 + carry;
		uint8_t result = temp & 0xFF;
		SetFlag(Flag::C, temp > 0xFF);
		SetFlag(Flag::Z, result == 0);
		SetFlag(Flag::N, 0);
		SetFlag(Flag::H, ((value1 & 0xF) + (value2 & 0xF) + carry > 0xF));
		writeOperand8(m_CurrentInstruction.operand1, result);
	}
}

void CPU::SUB()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	
	uint16_t temp = (uint16_t)AF.hi - (uint16_t)value;
	uint8_t result = (uint8_t)temp;
	SetFlag(Flag::C, AF.hi < value);
	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::N, 1);
	SetFlag(Flag::H, (AF.hi & 0xF) < (value & 0xF));

	//writeOperand8(m_CurrentInstruction.operand1, result);
	AF.hi = result;
}

void CPU::SBC()
{
	uint8_t value1 = fetch(m_CurrentInstruction.operand1);
	uint8_t value2 = fetch(m_CurrentInstruction.operand2);
	uint8_t carry = GetFlag(Flag::C);
	
	uint16_t temp = value1 - value2 - carry;
	uint8_t result = temp & 0xFF;

	SetFlag(Flag::C, (int16_t)temp < 0);
	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::N, 1);
	SetFlag(Flag::H, (value1 & 0xF) - (value2 & 0xF) - carry < 0);
	writeOperand8(m_CurrentInstruction.operand1, result);

}

void CPU::AND()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	AF.hi &= value;
	SetFlag(Flag::Z, AF.hi == 0);
	SetFlag(Flag::N, 0);
	SetFlag(Flag::H, 1);
	SetFlag(Flag::C, 0);
}

void CPU::XOR()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	AF.hi ^= value;
	SetFlag(Flag::Z, AF.hi == 0);
	SetFlag(Flag::N, 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::C, 0); 
}


void CPU::OR()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	AF.hi |= value;
	SetFlag(Flag::Z, AF.hi == 0);
	SetFlag(Flag::N, 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::C, 0); 
}

void CPU::CP()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	uint8_t result = AF.hi - value;
	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::N, 1);
	SetFlag(Flag::C, AF.hi < value);
	SetFlag(Flag::H, (AF.hi & 0xF) < (value & 0xF));
}


void CPU::RET()
{
	if(checkCond(m_CurrentInstruction.operand1.cond))
	{
		m_Cycles += 3; // 5 total cycles if we return

		PC = cpu_pop16();
	}
}

void CPU::POP()
{

	uint16_t value = cpu_pop16();

	if(m_CurrentInstruction.operand1.reg == RegType::AF) value &= 0xFFF0;

	writeRegister16(m_CurrentInstruction.operand1.reg, value); // always will be a register
}

void CPU::PUSH()
{
	uint16_t value = fetch(m_CurrentInstruction.operand1);

	cpu_push16(value);
}

void CPU::JP()
{
	uint16_t jumpAddress = fetch(m_CurrentInstruction.operand2);
	if(checkCond(m_CurrentInstruction.operand1.cond))
	{
		m_Cycles++;
		PC = jumpAddress;
	}
}

void CPU::CALL()
{
	uint16_t address = fetch(m_CurrentInstruction.operand2);
	if(checkCond(m_CurrentInstruction.operand1.cond))
	{
		m_Cycles += 3;

		cpu_push16(PC);

		PC = address;
	}
}

void CPU::RLCA()
{
	bool isBit7Set = (AF.hi & 0b10000000) != 0;
	AF.hi = (AF.hi << 1) | isBit7Set;

	SetFlag(Flag::C, isBit7Set);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
	SetFlag(Flag::Z, 0);
}

void CPU::RLA()
{
	uint8_t bit0 = GetFlag(Flag::C);
	uint8_t bit7 = (AF.hi & 0b10000000) != 0;
	AF.hi = (AF.hi << 1) | bit0;

	SetFlag(Flag::C, bit7);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
	SetFlag(Flag::Z, 0);
}

void CPU::RRCA()
{
	uint8_t bit0 = AF.hi & 1;
	AF.hi = (bit0 << 7) | (AF.hi >> 1);

	SetFlag(Flag::C, bit0);	
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
	SetFlag(Flag::Z, 0);
}

void CPU::RRA()
{
	uint8_t carry_in = GetFlag(Flag::C) ? 0x80 : 0;
	uint8_t carry_out = AF.hi & 0x01;

	AF.hi = (AF.hi >> 1) | carry_in;

	SetFlag(Flag::C, carry_out);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
	SetFlag(Flag::Z, 0); // Always cleared by RRA
}

void CPU::JR()
{
	int8_t rel_addr = fetch(m_CurrentInstruction.operand2);
	if(checkCond(m_CurrentInstruction.operand1.cond))
	{
		m_Cycles++;
		PC += rel_addr;
	}
}

void CPU::SCF()
{
	SetFlag(Flag::C, 1);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
}

void CPU::CPL()
{
	AF.hi ^= 0xFF;

	SetFlag(Flag::H, 1);
	SetFlag(Flag::N, 1);
}

void CPU::CCF()
{
	SetFlag(Flag::C, !GetFlag(Flag::C));
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
}

void CPU::STOP()
{
	//halted = true;
	// should stop LCD display aswell
}

void CPU::RETI()
{
	int_master_enabled = true;
	uint8_t lo = emu->read(SP++);
	uint8_t hi = emu->read(SP++);
	uint16_t address = ((uint16_t)hi << 8) | lo;
	PC = address;
}

void CPU::DI()
{
	int_master_enabled = false;
}

void CPU::EI()
{
	ime_enabling = true;
}

void CPU::RST()
{
	uint8_t target = m_CurrentInstruction.operand1.meta;
	cpu_push16(PC);

	uint16_t newAddress = target * 8;
	PC = newAddress;

}

void CPU::DAA()
{
	uint8_t u = 0;
	bool fc = false;

	if(GetFlag(Flag::H) || (!GetFlag(Flag::N) && (AF.hi & 0xF) > 9))
		u |= 0x06;

	if(GetFlag(Flag::C) || (!GetFlag(Flag::N) && (AF.hi > 0x99)))
	{
		u |= 0x60;
		fc = 1;
	}

	int a = AF.hi;

	if (GetFlag(Flag::N))
		a -= u;
	else
		a += u;

	AF.hi = static_cast<uint8_t>(a);

	SetFlag(Flag::Z, AF.hi == 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::C, GetFlag(Flag::N) ? GetFlag(Flag::C) : fc);

	
}


void CPU::RLC()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	bool carryOut = (value & (1 << 7)) ? 1 : 0;
	uint8_t result = (value << 1) | carryOut;

	writeOperand8(m_CurrentInstruction.operand1, result);

	SetFlag(Flag::C, carryOut);
	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
}

void CPU::RRC()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	uint8_t carryOut = value & 1;
	uint8_t result = (value >> 1) | (carryOut << 7);

	writeOperand8(m_CurrentInstruction.operand1, result);

	SetFlag(Flag::C, carryOut);
	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
}

void CPU::RL()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	uint8_t carryOut = (value & (1 << 7)) ? 1 : 0;
	uint8_t result = (value << 1) | GetFlag(Flag::C);

	writeOperand8(m_CurrentInstruction.operand1, result);

	SetFlag(Flag::C, carryOut);
	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
}

void CPU::RR()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	uint8_t carryOut = value & 1;
	uint8_t result = (value >> 1) | (GetFlag(Flag::C) << 7);

	writeOperand8(m_CurrentInstruction.operand1, result);

	SetFlag(Flag::C, carryOut);
	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
}

void CPU::SLA()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	uint8_t carryOut = (value & (1 << 7)) ? 1 : 0;
	uint8_t result = value << 1;

	writeOperand8(m_CurrentInstruction.operand1, result);

	SetFlag(Flag::C, carryOut);
	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
}

void CPU::SRA()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	uint8_t carryOut = value & 1;
	uint8_t result = (value >> 1) | (value & 0x80);

	writeOperand8(m_CurrentInstruction.operand1, result);

	SetFlag(Flag::C, carryOut);
	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
}

void CPU::SWAP()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);

	uint8_t result = ((value & 0xF) << 4) | (value >> 4);
	writeOperand8(m_CurrentInstruction.operand1, result);

	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::C, 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);
}

void CPU::SRL()
{
	uint8_t value = fetch(m_CurrentInstruction.operand1);
	
	uint8_t carryOut = value & 1;
	uint8_t result = (value >> 1);

	writeOperand8(m_CurrentInstruction.operand1, result);

	SetFlag(Flag::C, carryOut);
	SetFlag(Flag::Z, result == 0);
	SetFlag(Flag::H, 0);
	SetFlag(Flag::N, 0);

}

void CPU::BIT()
{
	uint8_t bit = m_CurrentInstruction.operand1.meta;
	uint8_t value = fetch(m_CurrentInstruction.operand2);
	uint8_t result = (value & (1 << bit)) ? 1 : 0;
	SetFlag(Flag::Z, !result);
	SetFlag(Flag::N, 0);
	SetFlag(Flag::H, 1);
}

void CPU::SET()
{
	uint8_t bit = m_CurrentInstruction.operand1.meta;
	uint8_t value = fetch(m_CurrentInstruction.operand2);
	uint8_t result = value | (1 << bit);
	writeOperand8(m_CurrentInstruction.operand2, result);
}

void CPU::RES()
{
	uint8_t bit = m_CurrentInstruction.operand1.meta;
	uint8_t value = fetch(m_CurrentInstruction.operand2);
	uint8_t result = value & ~(1 << bit);
	writeOperand8(m_CurrentInstruction.operand2, result);

}


// there has to be a better way of doing this
CPU::Instruction CPU::InstructionByOpcode(uint8_t opcode) 
{
	switch(opcode)
	{
	case 0x00:
		return {"NOP", 1, &CPU::NOP};
	case 0x07:
		return {"RLCA", 1, &CPU::RLCA};
	case 0x08:
	{
		// LD (imm16), SP
		return {"LD", 5, &CPU::LD, {IND_IMM16}, {REG16, RegType::SP}}; // when write operand will call write to IMM16 it will treat it as indirect
	}
	case 0x10:
		return {"STOP", 1, &CPU::STOP};
	case 0x17:
		return {"RLA", 1, &CPU::RLA};
	case 0x27:
		return {"DAA", 1, &CPU::DAA};
	case 0x76:
 		return {"HALT", 1, &CPU::HALT };
	case 0xC6:
		return {"ADD", 2, &CPU::ADD, {REG8, RegType::A}, {IMM8}};
	case 0xD6:
		return {"SUB", 2, &CPU::SUB, {IMM8}};
	case 0xE6:
		return {"AND", 2, &CPU::AND, {IMM8}}; 
	case 0xEE:
		return {"XOR", 2, &CPU::XOR, {IMM8}};
	case 0xF6:
		return {"OR", 2, &CPU::OR, {IMM8}};
	case 0xFE:
		return {"CP", 2, &CPU::CP, {IMM8}};
	case 0xC9:
		return {"RET", 1, &CPU::RET, {COND, RegType::NONE, CondType::NONE}}; // we already add 3 cycles if we jump, maybe the cycle count should be done like LD byt this is simplier for now
	case 0xD9:
		return {"RETI", 4, &CPU::RETI};
	case 0xC3:
		return {"JP", 3, &CPU::JP, {COND, RegType::NONE, CondType::NONE}, {IMM16}}; // should be 4 but we add 1
	case 0xE9:
		return {"JP", 0, &CPU::JP, {COND}, {REG16, RegType::HL}}; // JP already adds one
	case 0xEA:
		return {"LD", 4, &CPU::LD, {IND_IMM16}, {REG8, RegType::A}};
	case 0xE8:
		return {"ADD SP", 4, &CPU::ADD_SP, {IMM8}};
	case 0xFA:
		return {"LD", 4, &CPU::LD, {REG8, RegType::A}, {IND_IMM16}};
	case 0xE0:
		return {"LD", 3, &CPU::LD, {IND_IMM8}, {REG8, RegType::A}};
	case 0xF0:
		return {"LD", 3, &CPU::LD, {REG8, RegType::A}, {IND_IMM8}};
	case 0xF2:
		return {"LD", 2, &CPU::LD, {REG8, RegType::A}, {IND_REG8, RegType::C}};
	case 0xE2:
		return {"LD", 2, &CPU::LD, {IND_REG8, RegType::C}, {REG8, RegType::A}};
	case 0xCD:
		return {"CALL", 3, &CPU::CALL, {COND, RegType::NONE, CondType::NONE}, {IMM16}};
	case 0x18:
		return {"JR", 2, &CPU::JR, {COND, RegType::NONE, CondType::NONE}, {IMM8}};
	case 0x0F:
		return {"RRCA", 1, &CPU::RRCA};
	case 0x1F:
		return {"RRA", 1, &CPU::RRA};
	case 0x37:
		return {"SCF", 1, &CPU::SCF};
	case 0x2F:
		return {"CPL", 1, &CPU::CPL};
	case 0x3F:
		return {"CCF", 1, &CPU::CCF};
	case 0xF3:
		return {"DI", 1, &CPU::DI};
	case 0xFB:
		return {"EI", 1, &CPU::EI};
	case 0xCE:
		return {"ADC", 2, &CPU::ADC, {REG8, RegType::A}, {IMM8}};
	case 0xDE:
		return {"SBC", 2, &CPU::SBC, {REG8, RegType::A}, {IMM8}};
	case 0xF8:
		return {"LD HL, SP+", 3, &CPU::LD_HL_SP, {IMM8}}; // probably add operand for imm8
	case 0xF9:
		return {"LD", 2, &CPU::LD, {REG16, RegType::SP}, {REG16, RegType::HL}};
	case 0xCB:
		std::runtime_error("0xCB instruction, needs to be handled externally, ie. use HandleCBInstruction");
	

	default:
		if ((opcode & 0b11000000) == 0b01000000) // LD r8, r8
		{
			Operand dest = DecodeReg8((opcode & 0b00111000) >> 3);
			Operand src = DecodeReg8(opcode & 0b00000111);
			uint8_t cycles = (dest.mode == IND) || (src.mode == IND) ? 2 : 1;
			return { "LD", cycles, &CPU::LD, dest, src };

		}
		else if ((opcode & 0b11001111) == 0b00000001) // LD r16, imm16
		{
			Operand dest = DecodeReg16((opcode & 0b00110000) >> 4);
			Operand src = {IMM16};
			return {"LD", 3, &CPU::LD, dest, src};
		}
		else if ((opcode & 0b11000111) == 0b00000110) // LD r8, imm8
		{
			Operand dest = DecodeReg8((opcode & 0b00111000) >> 3);
			Operand src = {IMM8};
			return { "LD", 2, &CPU::LD, dest, src };
		}

		else if ((opcode & 0b11001111) == 0b00000010) // LD r16mem, a
		{
			Operand dest = DecodeReg16MEM((opcode & 0b00110000) >> 4);
			return { "LD", 2, &CPU::LD, dest, {REG8, RegType::A}};
		}
		else if ((opcode & 0b11001111) == 0b00001010) // LD a, r16mem
		{
			return {"LD", 4, &CPU::LD, {REG8, RegType::A}, DecodeReg16MEM((opcode & 0b00110000) >> 4)};
		}
		else if ((opcode & 0b11000111) == 0b00000100) // INC r8
		{
			Operand src = DecodeReg8((opcode & 0b00111000) >> 3);
			uint8_t cycles = src.mode == IND ? 3 : 1;
			return {"INC", cycles, &CPU::INC, src};
		}
		else if ((opcode & 0b11000111) == 0b00000101) // DEC r8
		{
			Operand src = DecodeReg8((opcode & 0b00111000) >> 3);
			uint8_t cycles = src.mode == IND ? 3 : 1;
			return {"DEC", cycles, &CPU::DEC, src};
		}
		else if ((opcode & 0b11001111) == 0b00000011) // INC r16
		{
			return {"INC", 2, &CPU::INC, DecodeReg16((opcode & 0b00110000) >> 4)};
		}
		else if ((opcode & 0b11001111) == 0b00001011) // DEC r16
		{
			return {"DEC", 2, &CPU::DEC, DecodeReg16((opcode & 0b00110000) >> 4)};
		}
		else if ((opcode & 0b11001111) == 0b00001001) // ADD HL, r16
		{
			return {"ADD", 2, &CPU::ADD, {REG16, RegType::HL}, DecodeReg16((opcode & 0b00110000) >> 4)};
		}
		else if ((opcode & 0b11111000) == 0b10000000) // ADD A, r8 // TODO: change cycle count when using (HL)
		{
			Operand src = DecodeReg8((opcode & 0b00000111));
			uint8_t cycles = src.mode == IND ? 2 : 1;
			return {"ADD", cycles, &CPU::ADD, {REG8, RegType::A}, src};
		}
		else if ((opcode & 0b11111000) == 0b10001000) // ADC A, r8
		{
			Operand src = DecodeReg8((opcode & 0b00000111));
			uint8_t cycles = src.mode == IND ? 2 : 1;
			return {"ADC", 1, &CPU::ADC, {REG8, RegType::A}, src};
		}
		else if ((opcode & 0b11111000) == 0b10010000) // SUB r8
		{
			Operand src = DecodeReg8((opcode & 0b00000111));
			uint8_t cycles = src.mode == IND ? 2 : 1;
			return {"SUB", cycles, &CPU::SUB, src};
		}
		else if ((opcode & 0b11111000) == 0b10011000) // SBC A, r8
		{
			Operand src = DecodeReg8((opcode & 0b00000111));
			uint8_t cycles = src.mode == IND ? 2 : 1;	
			return {"SBC", cycles, &CPU::SBC, {REG8, RegType::A}, src};
		}
		else if ((opcode & 0b11111000) == 0b10100000) // AND r8
		{
			Operand src = DecodeReg8((opcode & 0b00000111));
			uint8_t cycles = src.mode == IND ? 2 : 1;	
			return {"AND", cycles, &CPU::AND, DecodeReg8((opcode & 0b00000111))};
		}
		else if ((opcode & 0b11111000) == 0b10110000) // OR r8
		{
			Operand src = DecodeReg8((opcode & 0b00000111));
			uint8_t cycles = src.mode == IND ? 2 : 1;	
			return {"OR", cycles, &CPU::OR, src};
		}
		else if ((opcode & 0b11111000) == 0b10101000) // XOR r8
		{
			Operand src = DecodeReg8((opcode & 0b00000111));
			uint8_t cycles = src.mode == IND ? 2 : 1;	
			return {"XOR", cycles, &CPU::XOR, src};
		}
		else if ((opcode & 0b11111000) == 0b10111000) // CP r8
		{
			Operand src = DecodeReg8((opcode & 0b00000111));
			uint8_t cycles = src.mode == IND ? 2 : 1;	
			return {"CP", cycles, &CPU::CP, src};
		}
		else if((opcode & 0b11100111) == 0b11000000) // RET cond
		{
			return {"RET", 2, &CPU::RET, DecodeCond((opcode & 0b00011000) >> 3)};
		}
		else if((opcode & 0b11001111) == 0b11000001) // POP r16stk
		{
			return {"POP", 3, &CPU::POP, DecodeReg16STK((opcode & 0b00110000) >> 4)};
		}
		else if((opcode & 0b11001111) == 0b11000101) // PUSH r16stk
		{
			return {"PUSH", 4, &CPU::PUSH, DecodeReg16STK((opcode & 0b00110000) >> 4)};
		}
		else if((opcode & 0b111000111) == 0b11000010) // JP cond imm16  +1 cycle if jump
		{
			return {"JP", 3, &CPU::JP, DecodeCond((opcode & 0b00011000) >> 3), {IMM16}};
		}
		else if((opcode & 0b11100111) == 0b11000100) // CALL cond imm16  +3 cycle if jump
		{
			return {"CALL", 3, &CPU::CALL, DecodeCond((opcode & 0b00011000) >> 3), {IMM16}};
		}
		else if((opcode & 0b11100111) == 0b00100000) // JR cond s8   +1 cycle if jump
		{
			return {"JR", 2, &CPU::JR, DecodeCond((opcode & 0b00011000) >> 3), {IMM8}};
		}
		else if (((opcode & 0b11000111) == 0b11000111))
		{
			return {"RST", 4, &CPU::RST, {IMPL, RegType::NONE, CondType::NONE, (uint8_t)((opcode & 0b00111000) >> 3)}};
		}
		else 
		{
			//std::cout << "ERROR UNHANDLED OPCODE " << (int)opcode <<  std::endl;
		}
	}

	// incase of failure just do nothing
	// should throw an error
	return {"XXX", 1, &CPU::NOP};
}

CPU::Instruction CPU::HandleCBInstruction(uint8_t opcode)
{


	// TODO: change cycle count for HL
	if((opcode & 0b111111000) == 0b00000000) // rlc r8
		return {"RLC", 2, &CPU::RLC, DecodeReg8(opcode & 0b111)};
	else if((opcode & 0b111111000) == 0b00001000) // rrc r8
		return {"RRC", 2, &CPU::RRC, DecodeReg8(opcode & 0b111)};
	else if((opcode & 0b111111000) == 0b00010000) // rl r8
		return {"RL", 2, &CPU::RL, DecodeReg8(opcode & 0b111)};
	else if((opcode & 0b111111000) == 0b00011000) // rr sr8
		return {"RR", 2, &CPU::RR, DecodeReg8(opcode & 0b111)};
	else if((opcode & 0b111111000) == 0b00100000) // sla r8
		return {"SLA", 2, &CPU::SLA, DecodeReg8(opcode & 0b111)};
	else if((opcode & 0b111111000) == 0b00101000) // sra r8
		return {"SRA", 2, &CPU::SRA, DecodeReg8(opcode & 0b111)};
	else if((opcode & 0b111111000) == 0b00110000) // swap r8
		return {"SWAP", 2, &CPU::SWAP, DecodeReg8(opcode & 0b111)};
	else if((opcode & 0b111111000) == 0b00111000) // srl r8
		return {"SRL", 2, &CPU::SRL, DecodeReg8(opcode & 0b111)};
	else if((opcode & 0b11000000) == 0b01000000) // bit inx r8
		return {"BIT", 2, &CPU::BIT, {IMPL, RegType::NONE, CondType::NONE, (opcode & 0b00111000) >> 3}, DecodeReg8(opcode & 0b111)};
	else if((opcode & 0b11000000) == 0b10000000) // res inx r8
		return {"RES", 2, &CPU::RES, {IMPL, RegType::NONE, CondType::NONE, (opcode & 0b00111000) >> 3}, DecodeReg8(opcode & 0b111)};
	else if((opcode & 0b11000000) == 0b11000000) // bit inx r8
		return {"SET", 2, &CPU::SET, {IMPL, RegType::NONE, CondType::NONE, (opcode & 0b00111000) >> 3}, DecodeReg8(opcode & 0b111)};

	std::cout << "UNHANDLED CB INSTRUCTION " << (int)opcode << std::endl;
	return {"XXX", 1, &CPU::NOP};
}


CPU::Operand CPU::DecodeReg8(uint8_t bits)
{
	switch(bits)
	{
		case 0: return {REG8, RegType::B};
		case 1: return {REG8, RegType::C};
		case 2: return {REG8, RegType::D};
		case 3: return {REG8, RegType::E};
		case 4: return {REG8, RegType::H};
		case 5: return {REG8, RegType::L};
		case 6: return {IND,  RegType::HL};
		case 7: return {REG8, RegType::A};
	}
	std::cout << "ERROR DECODED INCORRECTLY" << std::endl;
	return {};
}

CPU::Operand CPU::DecodeReg16(uint8_t bits)
{
	switch(bits)
	{
		case 0: return {REG16, RegType::BC};
		case 1: return {REG16, RegType::DE};
		case 2: return {REG16, RegType::HL};
		case 3: return {REG16, RegType::SP};
	}
	std::cout << "ERROR DECODED INCORRECTLY" << std::endl;
	return {};
}

CPU::Operand CPU::DecodeReg16STK(uint8_t bits)
{
	switch(bits)
	{
		case 0: return {REG16, RegType::BC};
		case 1: return {REG16, RegType::DE};
		case 2: return {REG16, RegType::HL};
		case 3: return {REG16, RegType::AF};
	}
	std::cout << "ERROR DECODED INCORRECTLY" << std::endl;
	return {};
}

CPU::Operand CPU::DecodeReg16MEM(uint8_t bits)
{
	switch(bits)
	{
		case 0: return {IND, RegType::BC};
		case 1: return {IND, RegType::DE};
		case 2: return {IND, RegType::HLI};
		case 3: return {IND, RegType::HLD};
	}
	std::cout << "ERROR DECODED INCORRECTLY" << std::endl;
	return {};
}

CPU::Operand CPU::DecodeCond(uint8_t bits)
{
	switch (bits)
	{
	case 0: return {COND, RegType::NONE, CondType::NZ};
	case 1: return {COND, RegType::NONE, CondType::Z};
	case 2: return {COND, RegType::NONE, CondType::NC};
	case 3: return {COND, RegType::NONE, CondType::C};
	}

	std::cout << "DECODED CONDITION INCORRECTLY" << std::endl;
	return {};
}

bool CPU::checkCond(CondType cond)
{
	switch (cond)
	{
		case CondType::NONE: return true;
		case CondType::NZ: return !GetFlag(Flag::Z);
		case CondType::Z: return GetFlag(Flag::Z);
		case CondType::NC: return !GetFlag(Flag::C);
		case CondType::C: return GetFlag(Flag::C);
	}

	std::cout << "Invalid condition!!" << std::endl;
	return false;
}