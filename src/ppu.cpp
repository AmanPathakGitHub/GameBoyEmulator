#include "ppu.h"

#include "emulator.h"

PPU::PPU()
{
    
}

void PPU::tick()
{
    switch (mode)
    {
    case Mode::OAMSCAN:
        HandleModeOAMScan();
        break;
    
    case Mode::HBLANK:
        HandleModeHBLANK();
        break;

    case Mode::VBLANK:
        HandleModeVBLANK();
        break;
    case Mode::DRAWPIXELS:
        HandleModeDrawPixels();
        break;
    
    }

    dots++;
}

void PPU::ConnectLCD(LCD *lcd)
{
    this->lcd = lcd;
}

void PPU::HandleModeOAMScan()
{
     // search everything on the first dot, and just do nothing until 80 dots

    if(dots == 1) 
    {
        for(OAMEntry oam_sprite : oam_ram)
        {
            if(sprite_buffer.size() >= 10) break;
            if(oam_sprite.x > 0) break;
            if(!(lcd->ly + 16 >= oam_sprite.y)) break;
            if(!(lcd->ly + 16 < oam_sprite.y + (lcd->GetControlBit(LCDC_OBJ_SIZE) ? 16 : 8))) break;

            sprite_buffer.push_back(oam_sprite);
        }
    }
    else if(dots >= 80)
    {
        mode = DRAWPIXELS;
    }
}

void PPU::HandleModeHBLANK()
{
}

void PPU::HandleModeVBLANK()
{
}

void PPU::HandleModeDrawPixels()
{
    if(dots % 2 == 0)
        FetchBackGroundPixels();
}

void PPU::FetchBackGroundPixels()
{
    switch (fetch_state)
    {
    case GetTile:
    {
        uint16_t tileAddress = 0x9800;

        //if(lcd->GetControlBit(LCDC_BG_TILEMAP) && dots)
    }
        break;
    case DataLow:
        break;
    case DataHigh:
        break;
    case Sleep:
        break;
    case Push:
        break;    
    }
}

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

LCD::LCD()
{
    lcdc = 0x91;
    status = 0x81;
    scrollX = 0;
    scrollY = 0;
    ly = 0x91;
    lyc = 0;
    windowX = 0;
    windowY = 0;
}

uint8_t LCD::read(uint16_t address)
{
    if(address == 0xFF40)
        return lcdc;
    else if (address = 0xFF41)
        return status;
    else if(address == 0xFF44) // read only
        return ly;
    else if(address == 0xFF45)
        return lyc;
    else if(address == 0xFF4A) 
        return windowY;
    else if(address == 0xFF4B)
        return windowX;
    
    return 0xFF;
}

void LCD::write(uint16_t address, uint8_t data)
{
    if(address == 0xFF40)
        lcdc = data;
    else if(address == 0xFF41)
        status = data;
    else if(address == 0xFF45)
        lyc = data;
    else if(address == 0xFF4A) 
        windowY = data;
    else if(address == 0xFF4B)
        windowX = data;
    
}
