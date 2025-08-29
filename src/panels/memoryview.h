#pragma once

#include "panel.h"
#include "emulator.h"

class MemoryView : public Panel
{
public:
	MemoryView(Emulator& emu, const std::string& name);
	MemoryView(Emulator& emu, std::string&& name);

	void Update() override;
private:
	Emulator& m_Emulator;
};