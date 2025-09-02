#pragma once
#include <cstdint>
#include <array>
#include <string>
#include <memory>
#include <cstdint>


#include "cartridge.h"
#include "cpu.h"
#include "timer.h"
#include "ppu.h"

  

class Emulator
{
public:
	Emulator();

	void UpdateFrame(); // this updates all emulator things, including the buffer of pixels
	void clock();
	
	struct ButtonState
	{
		bool a = false;
		bool b = false;
		bool select = false;
		bool start = false;
		bool right = false;
		bool left = false;
		bool up = false;
		bool down = false;

		bool sel_dpad = false;
		bool sel_button = false;
	};

	ButtonState buttonState;
	void SetButtonState(uint8_t data);
	uint8_t GetButtonOutput();
	
	uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);

	uint16_t read16(uint16_t address);
	void write16(uint16_t address, uint16_t data);

	void LoadROM(const std::string& filepath);

	void Reset();

	CPU cpu; // public just to draw stuff
	Timer timer;
	PPU ppu;
	LCD lcd;
	DMA dma;
	
	std::array<uint8_t, 0x10000> memory;

	std::array<uint8_t, 0x2000> wram;
	std::array<uint8_t, 0x80> hram;


	bool romLoaded = false;
	uint64_t m_SystemTicks = 0;


private:
	std::unique_ptr<Cartridge> cartridge;


	uint8_t serial_data[2];
	uint8_t joypadState = 0x30;

};