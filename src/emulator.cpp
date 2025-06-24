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

	for (int i = 0; i < memory.size(); i++)
		memory[i] = 0x00;

	
	for (int i = 0; i < wram.size(); i++)
		wram[i] = 0x00;

	
	for (int i = 0; i < hram.size(); i++)
		hram[i] = 0x00;
	
	// memory[0x100] = 0x78; // LD A, B
	// memory[0x101] = 0x41; // LD B, C
	// memory[0x102] = 0x01; // LD BC, imm16
	// memory[0x103] = 0x05;
	// memory[0x104] = 0x02;
	// memory[0x105] = 0x3E; // LD A, imm8
	// memory[0x106] = 0xCC;
	// memory[0x107] = 0x02; // LD (BC), A

	// memory[0x100] = 0x08; // LD imm16, SP
	// memory[0x101] = 0x20;
	// memory[0x102] = 0x01;
	// memory[0x103] = 0x3E; // LD A, imm8
	// memory[0x104] = 0xFF;
	// memory[0x105] = 0x22; // LD (BC), A
	// memory[0x106] = 0x56; // LD D, L
	// memory[0x107] = 0x09; // ADD HL, BC

	// memory[0x100] = 0x03; // INC BC
	// memory[0x101] = 0x14; // INC D
	// memory[0x102] = 0x35; // DEC (HL)

	// memory[0x100] = 0x33; 
	// memory[0x101] = 0x3B; 
	// memory[0x102] = 0x39; 
	// memory[0x103] = 0xF9;
	// memory[0x104] = 0xE8;
	// memory[0x105] = 0x1; 
	// memory[0x106] = 0xE8;
	// memory[0x107] = 0xFF;
	// memory[0x108] = 0xF8;
	// memory[0x109] = 0x1;
	// memory[0x10A] = 0xF8;
	// memory[0x10B] = 0xFF; 
 
 
	memory[0x100] = 0x8F; // ADC A, A
	memory[0x101] = 0x88; // ADC A, B
	memory[0x102] = 0x99; // SBC A, C
	memory[0x103] = 0x9A; // SBC A, D

	// // Initialize registers and flags
	// cpu.AF.hi = 0x14;    // A = 0x14
	// cpu.BC.hi = 0xFF;    // B = 0xFF
	// cpu.BC.lo = 0x02;    // C = 0x02
	// cpu.DE.hi = 0x03;    // D = 0x03

	// cpu.SetFlag(FLAG_C, 1); // Set carry flag for ADC/SBC

	// cpu.PC = 0x100;
 


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
	// system clocks MAX_CYCLES amount of times before drawing the screen.
	// clock speed = 4.194304MHz; 4194304 clocks a second
	// if we are doing 60fps 1 frame takes 1/60s that means we need to do 4194304/60 (69905) clocks before we draw the screen
	for(int i = 0; i < 256; i++)
	{
		memory[i] = i;
	}

	const int MAX_CYCLES = 69905;

	static std::string msg;


	for (int i = 0; i < MAX_CYCLES; i++)
	{
		clock();
		static int index = 0;
		if(read(0xFF02) == 0x81)
		{
			char c = read(0xFF01);
			msg += c;
			std::cout << c << std::flush;
			write(0xFF02, 0);
			//std::cout << "WRITTEN " << c << " AT PC: " << cpu.PC << std::endl;
			
		}
	}

	
}

void Emulator::clock()
{
	// using M-cycles	
	cpu.Clock();
	
	for(int i = 0; i <= 4; i++)
	{
		timer.tick(); // maybe pass in cpu to timer tick?
	}
		
	if(dma.isTransferring())
		dma.Tick();
	
	m_SystemTicks++;
}

void Emulator::clock_complete()
{
	while (cpu.m_Cycles != 0)
	{
		cpu.Clock();
	}


}

uint8_t ly = 0;

uint8_t Emulator::read(uint16_t address)
{
	//std::cout << "READ: " <<  (int)address << std::endl;

	if(address == 0xFF44) return ly++;

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
        //TODO
		if (address == 0xFF01)  serial_data[0] = data;
		else if (address == 0xFF02) serial_data[1] = data;
		else if (address >= 0xFF04 && address <= 0xFF07) timer.write(address, data);
		else if(address == 0xFF46) dma.StartTransfer(data);
		else if(address == 0xFF0F) cpu.int_flag = data;
        
    } else if (address == 0xFFFF) {
        //CPU SET ENABLE REGISTER
        
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
}
