#pragma once
#include <cstdint>
#include <array>
#include <string>
#include <memory>
#include <cstdint>


#include "cartridge.h"
#include "cpu.h"


class Emulator
{
public:
	Emulator();

	void UpdateFrame(); // this updates all emulator things, including the buffer of pixels
	void clock();
	
	uint8_t read(uint16_t address);
    void clock_complete();
    void write(uint16_t address, uint8_t data);
    //void write(uint16_t address, uint16_t data);


	uint16_t read16(uint16_t address);
	void write16(uint16_t address, uint16_t data);

	void LoadROM(const std::string& filepath);

	CPU cpu; // public just to draw stuff
	std::array<uint8_t, 0x10000> memory;
	uint32_t m_SystemTicks = 0;
private:
	std::unique_ptr<Cartridge> cartridge;

	uint8_t serial_data[2];

};