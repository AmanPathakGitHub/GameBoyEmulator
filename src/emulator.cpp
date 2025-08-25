#include "emulator.h"
#include "application.h"
#include <iostream>

Emulator::Emulator()
{
	cpu.Reset();
	cpu.ConnectCPUToBus(this);

	timer.ConnectTimerToCPU(&cpu);
	dma.ConnectToEmulator(this);
	ppu.ConnectLCD(&lcd);
	ppu.ConnectCPU(&cpu);
	ppu.ConnectToEmulator(this);
	lcd.ConnectToEmulator(this);

	Reset();

}

void Emulator::Reset()
{
	m_SystemTicks = 0;
	cpu.Reset();
	lcd.Reset();
	ppu.Reset();
	timer.Reset();

	for (int i = 0; i < memory.size(); i++)
		memory[i] = 0x00;

	for (int i = 0; i < wram.size(); i++)
		wram[i] = 0x00;

	for (int i = 0; i < hram.size(); i++)
		hram[i] = 0x00;


	serial_data[0] = 0;
	serial_data[1] = 0;

	memory[0xFF05] = 0x00; // TIMA
	memory[0xFF06] = 0x00; // TMA
	memory[0xFF07] = 0x00; // TAC
	memory[0xFF10] = 0x80; // NR10
	memory[0xFF11] = 0xBF;
	memory[0xFF12] = 0xF3;
	memory[0xFF14] = 0xBF;
	memory[0xFF16] = 0x3F;
	memory[0xFF17] = 0x00;
	memory[0xFF19] = 0xBF;
	memory[0xFF1A] = 0x7F;
	memory[0xFF1B] = 0xFF;
	memory[0xFF1C] = 0x9F;
	memory[0xFF1E] = 0xBF;
	memory[0xFF20] = 0xFF;
	memory[0xFF21] = 0x00;
	memory[0xFF22] = 0x00;
	memory[0xFF23] = 0xBF;
	memory[0xFF24] = 0x77;
	memory[0xFF25] = 0xF3;
	memory[0xFF26] = 0xF1; // Enable sound
	memory[0xFF40] = 0x91; // LCDC
	memory[0xFF42] = 0x00;
	memory[0xFF43] = 0x00;
	memory[0xFF45] = 0x00;
	memory[0xFF47] = 0xFC;
	memory[0xFF48] = 0xFF;
	memory[0xFF49] = 0xFF;
	memory[0xFF4A] = 0x00;
	memory[0xFF4B] = 0x00;
	memory[0xFFFF] = 0x00; // IE
}

void Emulator::UpdateFrame()
{
	if (!romLoaded) return;

	// system clocks MAX_CYCLES amount of times before drawing the screen.
	// clock speed = 4.194304MHz; 4194304 clocks a second
	// if we are doing 60fps 1 frame takes 1/60s that means we need to do 4194304/60 (69905) clocks before we draw the screen

	const static int MAX_CYCLES = 69905;

	static std::string msg;


	for (int i = 0; i < MAX_CYCLES; i++)
	{


		clock();

		if(read(0xFF02) == 0x81) // serial reading
		{
			char c = read(0xFF01);
			msg += c;
			std::cout << c << std::flush;
			write(0xFF02, 0);
			
		}
	}

	
}

void Emulator::clock()
{
	// using T-cycles	


	if (m_SystemTicks % 4 == 0)
	{
		cpu.Clock();

		
	}
	timer.tick(); // maybe pass in cpu to timer tick?
	
	if (dma.isTransferring())
		dma.Tick();

	ppu.tick();



	m_SystemTicks++;
}


void Emulator::SetButtonState(uint8_t data)
{
	buttonState.sel_dpad = data & 0x10;
	buttonState.sel_button = data & 0x20;
}

uint8_t Emulator::GetButtonOutput()
{
	uint8_t output = 0xCF;
	// if the bit is ZERO

	if (buttonState.sel_button == 0) 
	{
		if (buttonState.select) output &= ~(1 << 2);
		if (buttonState.start) output &= ~(1 << 3);
		if (buttonState.b) output &= ~(1 << 1);
		if (buttonState.a) output &= ~(1 << 0);
	}

	if (buttonState.sel_dpad == 0)
	{
		if (buttonState.up) output &= ~(1 << 2);
		if (buttonState.down) output &= ~(1 << 3);
		if (buttonState.left) output &= ~(1 << 1);
		if (buttonState.right) output &= ~(1 << 0);
	}

	return output;
}

uint8_t Emulator::read(uint16_t address)
{
	//std::cout << "READ: " <<  (int)address << std::endl;

	if (address < 0x8000) {
		return cartridge->ReadCart(address);
	} else if (address < 0xA000) {
		//PPU/VRAM
		return ppu.VRAM_read(address);
    } else if (address < 0xC000) {
        //Cartridge RAM
        return cartridge->ReadCart(address);
    } else if (address < 0xE000) {
        //WRAM (Working RAM)
        return wram[address - 0xC000];
    } else if (address < 0xFE00) {
        //reserved echo ram...
        return 0;
    } else if (address < 0xFEA0) {
        //OAM
		if(dma.isTransferring()) return 0xFF;
        return ppu.OAM_read(address);
    } else if (address < 0xFF00) {
        //reserved unusable...
        return 0;
    } else if (address < 0xFF80) {
        //IO Registers...
        //TODO

		if (address == 0xFF00)
			return GetButtonOutput();

		if (address == 0xFF01) {
        	return serial_data[0];
   		 }

    	if (address == 0xFF02) {
        	return serial_data[1];
    	}

	 	if (address >= 0xFF04 && address <= 0xFF07) 
			return timer.read(address);

		if(address == 0xFF0F)
			return cpu.int_flag;

		if (address >= 0xFF40 && address <= 0xFF4B)
			return lcd.read(address);
		
		

		return 0;
 
		//NO_IMPL
    } else if (address == 0xFFFF) {
        //CPU ENABLE REGISTER...
        //TODO
        return cpu.int_enable;
    }

    //NO_IMPL
    return hram[address - 0xFF80];	
}

void Emulator::write(uint16_t address, uint8_t data)
{
	//std::cout << "WRITE: " <<  (int)address << std::endl;

	if (address < 0x8000) {
        //ROM Data
        cartridge->WriteCart(address, data);
    } else if (address < 0xA000) {
		ppu.VRAM_write(address, data);
    } else if (address < 0xC000) {
        //EXT-RAM
        cartridge->WriteCart(address, data);
    } else if (address < 0xE000) {
        //WRAM
        wram[address - 0xC000] = data;
    } else if (address < 0xFE00) {
        //reserved echo ram
    } else if (address < 0xFEA0) {
		//OAM
		if(dma.isTransferring()) return;
		
		ppu.OAM_write(address, data);
    } else if (address < 0xFF00) {
        //unusable reserved
    } else if (address < 0xFF80) {
        //IO Registers...
		if (address == 0xFF00)
			SetButtonState(data);
		else if (address == 0xFF01)  serial_data[0] = data;
		else if (address == 0xFF02) serial_data[1] = data;
		else if (address >= 0xFF04 && address <= 0xFF07) timer.write(address, data);
		else if (address == 0xFF0F) cpu.int_flag = data;
		else if (address >= 0xFF40 && address <= 0xFF4B) lcd.write(address, data);

        
    } else if (address == 0xFFFF) {        
        cpu.int_enable = data;
    } else {
        hram[address - 0xFF80] = data;
    }
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
	romLoaded = true;

}
