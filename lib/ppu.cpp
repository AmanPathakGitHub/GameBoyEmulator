#include "ppu.h"

#include "emulator.h"
#include <iostream>
#include <cstring>
#include <algorithm>

PPU::PPU()
{
    memset(vram, 0, 0x2000);
    memset(videoBuffer, 3, RESX * RESY * 4);
    memset(oam_ram, 0, sizeof(OAMEntry) * 40);

    background_pixels = {};
    sprite_buffer.reserve(10);

}

void PPU::Reset()
{
    memset(vram, 0, 0x2000);
    memset(videoBuffer, 0, RESX * RESY);
    memset(oam_ram, 0, sizeof(OAMEntry) * 40);

    for (int i = 0; i < sprite_pixels.size(); i++)
        sprite_pixels[i] = {};

    background_pixels = {};
    if (lcd != nullptr)
    {
        lcd->ly = 0;
        SwitchMode(OAMSCAN);
    }


    sprite_buffer.clear();
    

    dots = 0;
    scanlineX = 0; // this is the x position in the scanline, used for pixel drawing
    pushedX = 0;

    windowLineCounter = 0;

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

void PPU::ConnectCPU(CPU* cpu)
{
    this->cpu = cpu;
}

void PPU::ConnectLCD(LCD *lcd)
{
    this->lcd = lcd;
    lcd->ly = 0;
    SwitchMode(OAMSCAN); // can't do it in constructor, lcd would be null
}

void PPU::ConnectToEmulator(Emulator* emu)
{
    this->emu = emu;
}

void PPU::SwitchMode(Mode mode)
{
    this->mode = mode;
    lcd->SetStatusBit(LCD::Status::PPUMODE, mode);
}

void PPU::HandleModeOAMScan()
{

    if(dots == 1) 
    {
        for(OAMEntry& oam_sprite : oam_ram)
        {
            if(sprite_buffer.size() >= 10) break;

            uint8_t spriteHeight = lcd->GetControlBit(LCD::Control::OBJ_SIZE) ? 16 : 8;
            if (!(lcd->ly + 16 < oam_sprite.y + spriteHeight)) continue;
            if(oam_sprite.x <= 0) continue;
            if(!(lcd->ly + 16 >= oam_sprite.y)) continue;

            sprite_buffer.emplace_back(oam_sprite, spriteHeight);
            
        }

        std::sort(sprite_buffer.begin(), sprite_buffer.end(), [](const Sprite& sprite1, const Sprite& sprite2)
            {
                return sprite1.spriteData.x < sprite2.spriteData.x;
            });
        
    }
    else if(dots >= 80)
    {        
        fetchedX = 0;
        scanlineX = 0;
        pushedX = 0;
        fetch_state = GetTile;
        SwitchMode(DRAWPIXELS);
    }

}

void PPU::HandleModeHBLANK()
{

    if (dots >= 456)
    {
        IncrementLY();


        if (lcd->ly >= RESY)
        {
            windowLineCounter = 0;
            SwitchMode(VBLANK);

            emu->cpu.RequestInterrupt(CPU::Interrupt::VBLANK);

            if (lcd->GetStatusBit(LCD::Status::MODE1)) emu->cpu.RequestInterrupt(CPU::Interrupt::STAT);

        } 
        else
        {
            SwitchMode(OAMSCAN);
            sprite_buffer.clear();
            if (lcd->GetStatusBit(LCD::Status::MODE2))  emu->cpu.RequestInterrupt(CPU::Interrupt::STAT);
        }

        dots = 0;

    }

}

void PPU::HandleModeVBLANK()
{
    if (dots >= 456)
    {
        IncrementLY();

        if (lcd->ly >= 154)
        {
            lcd->ly = 0;
            SwitchMode(OAMSCAN);
            windowTriggered = false;
            if (lcd->GetStatusBit(LCD::Status::MODE2)) emu->cpu.RequestInterrupt(CPU::Interrupt::STAT);
        }
        dots = 0;

    }

}

void PPU::HandleModeDrawPixels()
{

    if (dots % 10 == 0)
    {
        FetchBackGroundPixels();


    }

    FetchSpritePixels();



    PixelRender();

    if (pushedX >= RESX)
    {
        if (lcd->ly == lcd->lyc)
        {
            lcd->SetStatusBit(LCD::Status::LYC_LY, 1);

            if (lcd->GetStatusBit(LCD::Status::LYC)) emu->cpu.RequestInterrupt(CPU::Interrupt::STAT);
        }
        SwitchMode(HBLANK);

        while (!background_pixels.empty())
            background_pixels.pop();

        sprite_pixels.fill({});


        if (lcd->GetStatusBit(LCD::Status::MODE0)) emu->cpu.RequestInterrupt(CPU::Interrupt::STAT);

    }   
}

void PPU::FetchSpritePixels()
{
    if (!lcd->GetControlBit(LCD::Control::OBJ_ENABLE)) return;

    for (int i = 0; i < sprite_buffer.size(); i++)
    {
        
        uint8_t tileHeight = lcd->GetControlBit(LCD::Control::OBJ_SIZE) ? 16 : 8;
        bool isTallSprite = tileHeight == 16;
        const OAMEntry& sprite = sprite_buffer[i].spriteData;  
        
        int tileY = lcd->ly - (sprite.y - 16);

        if ((sprite.x - 8 <= scanlineX) && tileY >= 0 && tileY < tileHeight)
        {
            uint8_t tileIndex = sprite.tile_index;

            if (isTallSprite) tileIndex &= ~(1); // Index can only be even numbers when using 8x16

            if (sprite.f_yflip)
                tileY = tileHeight - 1 - tileY;                
           

            uint16_t tileAddress = 0x8000 + (tileIndex * 16) + (tileY * 2);

            uint8_t lo = emu->read(tileAddress);
            uint8_t hi = emu->read(tileAddress + 1);


            for (int i = 0; i < 8; i++)
            {
                uint8_t bit = sprite.f_xflip ? i : 7 - i;
                uint8_t hiBit = !!(hi & (1 << bit));
                uint8_t loBit = !!(lo & (1 << bit));
                
                uint8_t col = (hiBit << 1) | loBit;

                uint8_t index = scanlineX + i;

                if (scanlineX + i >= sprite_pixels.size()) break;

                if(col != 0 && !sprite_pixels[scanlineX + i].exists)
                    sprite_pixels[scanlineX + i] = OBJPixel{ true, col, (uint8_t)sprite.f_dmg_pallete, (uint8_t)sprite.f_priority };
            }

           sprite_buffer[i] = {}; // remove from sprite buffer
        }
    }
}

bool PPU::WindowVisible()
{
    uint8_t wx = lcd->windowX;
    uint8_t wy = lcd->windowY;

    return lcd->GetControlBit(LCD::Control::WINDOW_ENABLE) && wx >= 0 && wx < RESX && wy >= 0 && wy < RESY;
}

void PPU::FetchWindowPixels()
{
    if (lcd->ly < lcd->windowY || scanlineX < lcd->windowX - 14) // -14 is because of a hardware quirk/timing of when pixels are pushed
        return;
    
    uint16_t tileIndexAddress = lcd->GetControlBit(LCD::Control::WINDOW_TILEMAP) ? 0x9C00 : 0x9800;

    uint16_t fetcherX = ((fetchedX) - (lcd->windowX - 7) / 8) & 0x1F;
    uint16_t fetcherY = (lcd->ly - lcd->windowY);
    tileIndexAddress += 32 * (fetcherY / 8) + fetcherX;
    uint8_t tileIndex = emu->read(tileIndexAddress);

    tileAddress = lcd->GetControlBit(LCD::Control::BG_WINDOW_TILES) ? 0x8000 + tileIndex * 16 : 0x9000 + (int8_t)tileIndex * 16;

}

void PPU::FetchBackGroundPixels()
{

    if (lcd->GetControlBit(LCD::Control::BG_WINDOW_ENABLE))
    {
        // wrap in function
        uint16_t tileIndexAddress = lcd->GetControlBit(LCD::Control::BG_TILEMAP) ? 0x9C00 : 0x9800;

        uint16_t fetcherX = (lcd->scrollX / 8 + fetchedX) & 0x1F;
        uint16_t fetcherY = (lcd->scrollY + lcd->ly) & 255;

        tileIndexAddress += fetcherY / 8 * 32 + fetcherX;
        uint8_t tileIndex = emu->read(tileIndexAddress);
        tileAddress = lcd->GetControlBit(LCD::Control::BG_WINDOW_TILES) ? 0x8000 + tileIndex * 16 : 0x9000 + (int8_t)tileIndex * 16;

        if (WindowVisible())
            FetchWindowPixels();


    }

    fetchedX++;

    uint8_t tileY = ((lcd->ly + lcd->scrollY) % 8) * 2;
    tileLo = emu->read(tileAddress + tileY);
    tileHi = emu->read(tileAddress + tileY + 1);



    if (background_pixels.size() < 8)
    {

        for (int i = 7; i >= 0; i--)
        {
            uint8_t hi = !!(tileHi & (1 << i));
            uint8_t lo = !!(tileLo & (1 << i));

            uint8_t col = (hi << 1) | lo;

            if (!lcd->GetControlBit(LCD::Control::BG_WINDOW_ENABLE))
                col = 0;

            background_pixels.emplace(lcd->GetColor(col, lcd->bgp));


        }
    }


}

void PPU::PixelRender()
{
    // need 8 pixels at least or something
    if (background_pixels.empty() || lcd->ly >= RESY) return;

    uint8_t color = background_pixels.front();
    background_pixels.pop();

    
    OBJPixel spritePixel = sprite_pixels[pushedX];

    if (spritePixel.exists && !(spritePixel.background_priority == 1 && color != 0))
    {

        uint8_t pallete = spritePixel.pallete ? lcd->obp1 : lcd->obp0;
        color = lcd->GetColor(spritePixel.color, pallete);
    }
  
    int index = lcd->ly * RESX + pushedX;

    if (WindowVisible() && !(lcd->ly < lcd->windowY || scanlineX < lcd->windowX - 14))
    {
        if (scanlineX >= lcd->windowX % 8 || lcd->ly >= lcd->windowY % 8) // this needs to check window x and window y for smooth window scrolling
        {

            uint32_t defaultColors[4] = { 0xFFFFFFFF, 0xA9A9A9FF, 0x545454FF, 0x00000000 };

            videoBuffer[index] = defaultColors[color];
            pushedX++;
        }

    }
    else
    {
        if (scanlineX >= lcd->scrollX % 8) // this needs to check window x and window y for smooth window scrolling
        {

            uint32_t defaultColors[4] = { 0xFFFFFFFF, 0xA9A9A9FF, 0x545454FF, 0x00000000 };

            videoBuffer[index] = defaultColors[color];
            pushedX++;
        }

    }
    
    
    scanlineX++;


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

    emu->ppu.OAM_write(destinationAddress, data);

    transferring = destinationAddress < 0xFE9F;
    currentAddress++;
}

LCD::LCD()
{
    Reset();
}

void LCD::Reset()
{
    lcdc = 0x91;
    status = 0x81;
    scrollX = 0;
    scrollY = 0;
    ly = 0x91;
    lyc = 0;
    windowX = 0;
    windowY = 0;
    bgp = 0xFC;
}

uint8_t LCD::read(uint16_t address)
{
    if (address == 0xFF40)
        return lcdc;
    else if (address == 0xFF41)
        return status;
    else if (address == 0xFF42)
        return scrollY;
    else if (address == 0xFF43)
        return scrollX;
    else if (address == 0xFF44) // read only
        return ly;
    else if (address == 0xFF45)
        return lyc;
    else if (address == 0xFF47)
        return bgp;
    else if (address == 0xFF48)
        return obp0;
    else if (address == 0xFF49)
        return obp1;
    else if(address == 0xFF4A) 
        return windowY;
    else if(address == 0xFF4B)
        return windowX;
    
    return 0xFF;
}

void LCD::write(uint16_t address, uint8_t data)
{
    if (address == 0xFF40)
        lcdc = data;
    else if (address == 0xFF41)
        status = data;
    else if (address == 0xFF45)
        lyc = data;
    else if (address == 0xFF46)
        emu->dma.StartTransfer(data);
    else if (address == 0xFF42)
        scrollY = data;
    else if (address == 0xFF43)
        scrollX = data;
    else if (address == 0xFF47)
        bgp = data;
    else if (address == 0xFF48)
        obp0 = data;
    else if (address == 0xFF49)
        obp1 = data;
    else if(address == 0xFF4A) 
        windowY = data;
    else if(address == 0xFF4B)
        windowX = data;
    
}

void LCD::ConnectToEmulator(Emulator* emu)
{
    this->emu = emu;
}

void LCD::SetStatusBit(const LCD::Status statusType, uint8_t set)
{
    if (statusType == Status::PPUMODE)
    {
        status = (status & ~(0b11)) | set;
        return;
    }
        
    if (set) status |= statusType;
    else status &= ~(statusType);
}

void PPU::IncrementLY()
{
    lcd->ly++;
 
    if (WindowVisible() && lcd->ly > lcd->windowY)
        windowLineCounter++;

    if (lcd->ly == lcd->lyc)
    {
        lcd->SetStatusBit(LCD::Status::LYC_LY, 1);

        if (lcd->GetStatusBit(LCD::Status::LYC)) emu->cpu.RequestInterrupt(CPU::Interrupt::STAT);
    }
    else
        lcd->SetStatusBit(LCD::Status::LYC_LY, 0);
}

uint8_t LCD::GetColor(uint8_t index, uint8_t pallete)
{
    uint8_t colorID = (pallete & (0b11 << (index * 2))) >> (index * 2);

    uint8_t gb_colors[4] = {0, 1, 2, 3};

    return gb_colors[colorID];
}
