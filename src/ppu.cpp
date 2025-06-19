#include "ppu.h"

#include "emulator.h"


void PPU::OAM_write(uint16_t address, uint8_t data)
{
    if(address >= 0xFE00) address -= 0xFE00;

    uint8_t* oam_bytes = (uint8_t*)oam_ram;

    oam_bytes[address] = data;
}

uint8_t PPU::OAM_read(uint16_t address)
{
    if(address >= 0xFE00) address -= 0xFE00;

    uint8_t* oam_bytes = (uint8_t*)oam_ram;

    return oam_bytes[address];
}

void PPU::VRAM_write(uint16_t address, uint8_t data)
{
    vram[address - 0x8000] = data;
}

uint8_t PPU::VRAM_read(uint16_t address)
{
    return vram[address - 0x8000];
}

void DMA::ConnectToEmulator(Emulator *emu)
{
    this->emu = emu;
}

void DMA::StartTransfer(uint8_t value)
{
    currentAddress = (uint16_t)value << 8;
    transferring = true;
}

void DMA::Tick()
{
    uint8_t data = emu->read(currentAddress);
    uint16_t destinationAddress = 0xFE00 | (currentAddress & 0xFF);
    emu->write(destinationAddress, data);

    transferring = (currentAddress & 0xFF) < 0x9F;
    currentAddress++;
}
