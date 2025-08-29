#pragma once

#include "panel.h"
#include "emulator.h"

#include <sstream>

class Disassembler : public Panel
{
public:
	Disassembler(Emulator& emu, const std::string& name);
	Disassembler(Emulator& emu, std::string&& name);

	void Update() override;

private:
	Emulator& m_Emulator;

	std::vector<std::string> disassemble(Emulator& emu, uint16_t startAddress, uint16_t endAddress);
	void WriteParams(Emulator& emu, CPU::Operand& op, std::stringstream& ss, uint16_t& currentAddress);

};