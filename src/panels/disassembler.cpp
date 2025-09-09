#include "disassembler.h"

#include "imgui.h"
#include <sstream>

Disassembler::Disassembler(Emulator& emu, const std::string& name)
	: Panel(name), m_Emulator(emu)
{

}

Disassembler::Disassembler(Emulator& emu, std::string&& name)
	: Panel(std::move(name)), m_Emulator(emu)
{
}

void Disassembler::Update()
{
	if (!m_Emulator.romLoaded) return;

	std::vector<std::string> disassemblyLines = disassemble(m_Emulator, m_Emulator.cpu.PC - 10, m_Emulator.cpu.PC + 15);

	for (std::string l : disassemblyLines)
		ImGui::Text(l.c_str());

}

std::vector<std::string> Disassembler::disassemble(Emulator& emu, uint16_t startAddress, uint16_t endAddress)
{
	std::vector<std::string> output;
	for (uint16_t currentAddress = startAddress; currentAddress <= endAddress;)
	{
		uint8_t opcode = emu.read(currentAddress++);
		CPU::Instruction currentInstruction;
		if (opcode == 0xCB)
		{
			opcode = emu.read(currentAddress++);
			currentInstruction = emu.cpu.m_CBPrefixJumpTable[opcode];

		}
		else
			currentInstruction = emu.cpu.m_JumpTable[opcode];

		std::stringstream ss;

		if (currentAddress == emu.cpu.PC)
			ss << "***";

		ss << std::hex << currentAddress - 1 << " " << std::hex << (int)opcode << " " << currentInstruction.name;

		WriteParams(emu, currentInstruction.operand1, ss, currentAddress);
		WriteParams(emu, currentInstruction.operand2, ss, currentAddress);

		output.push_back(ss.str());
	}

	// std::cout << output[0] << std::endl;
	return output;
}

void Disassembler::WriteParams(Emulator& emu, CPU::Operand& op, std::stringstream& ss, uint16_t& currentAddress)
{
	switch (op.mode)
	{
	case CPU::AddressingMode::IMM8:
		ss << " " << std::hex << (int)emu.read(currentAddress++);
		break;
	case CPU::AddressingMode::IMM16:
		ss << " " << std::hex << (int)emu.read16(currentAddress);
		currentAddress += 2;
		break;

	case CPU::AddressingMode::REG16:
	case CPU::AddressingMode::REG8:
		ss << " " << RegTypeToString(op.reg);
		break;
	case CPU::AddressingMode::IND:
		ss << " (" << RegTypeToString(op.reg) << ")";
		break;
	case CPU::AddressingMode::IND_IMM8:
		ss << " (" << std::hex << (int)emu.read(currentAddress++) << ")";
		break;
	case CPU::AddressingMode::IND_IMM16:
		ss << " (" << std::hex << (int)emu.read16(currentAddress) << ")";
		currentAddress += 2;
		break;
	case CPU::AddressingMode::COND:
		ss << " " << CondTypeToString(op.cond);

	}
}