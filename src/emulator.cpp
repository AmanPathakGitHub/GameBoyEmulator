#include "emulator.h"

Emulator::Emulator()
{
	cpu.Reset();
	cpu.ConnectCPUToBus(this);

	for (int i = 0; i < memory.size(); i++)
		memory[i] = 0x00;
	
	//memory[0x100] = 0x78; // LD A, B
	//memory[0x101] = 0x41; // LD B, C
	//memory[0x102] = 0x01; // LD BC, imm16
	//memory[0x103] = 0xAB;
	//memory[0x104] = 0xBC;

	memory[0x100] = 0x1A; // LD imm16, SP
	memory[0x101] = 0x20;
	memory[0x102] = 0x01;
	memory[0x103] = 0x3E; // LD A, imm8
	memory[0x104] = 0xFF;
	memory[0x105] = 0x22; // LD (BC), A
	memory[0x106] = 0x56; // LD D, L

}

void Emulator::UpdateFrame()
{
	// system clocks MAX_CYCLES amount of times before drawing the screen.
	// clock speed = 4.194304MHz; 4194304 clocks a second
	// if we are doing 60fps 1 frame takes 1/60s that means we need to do 4194304/60 (69905) clocks before we draw the screen

	const int MAX_CYCLES = 69905;

	for (int i = 0; i < MAX_CYCLES; i++)
		clock();
}

void Emulator::clock()
{
	cpu.Clock();
	m_SystemTicks++;
}

uint8_t Emulator::read(uint16_t address)
{

	/*if (address < 0x8000)
		return cartridge->ReadCart(address);*/
	
	return memory[address];
}

void Emulator::write(uint16_t address, uint8_t data)
{
	/*if (address < 0x8000)
	{
		cartridge->WriteCart(address, data);
	}*/

	memory[address] = data;

}

uint16_t Emulator::read16(uint16_t address)
{
	// TODO handle endianess

	uint8_t lo = read(address);
	uint8_t hi = read(address + 1);

	return lo | (hi << 8);
}

void Emulator::write16(uint16_t address, uint16_t data)
{
	uint8_t lo = data & 0xFF;
	uint8_t hi = data >> 8;

	write(address, lo);
	write(address + 1, hi);
}

void Emulator::LoadROM(const std::string& filepath)
{
	cartridge = std::make_unique<Cartridge>(filepath);
}
