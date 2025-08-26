#pragma once

#include <cstdint>
#include <queue>
#include <array>
#include "cpu.h"

static const int RESX = 160;
static const int RESY = 144;

struct OAMEntry
{
    uint8_t y;
    uint8_t x;
    uint8_t tile_index;

    uint8_t f_palette : 3;      // CGB ONLY
    uint8_t f_bank : 1;         // CGB ONLY
    uint8_t f_dmg_pallete : 1;
    uint8_t f_xflip : 1;
    uint8_t f_yflip: 1;
    uint8_t f_priority : 1;
};

struct Sprite // USELESS NOW
{
    OAMEntry spriteData;
    uint8_t spriteHeight = 8;


};

class Emulator;

class DMA
{
public:
    void ConnectToEmulator(Emulator* emu);
    void StartTransfer(uint8_t value);
    void Tick();
    inline bool isTransferring() const { return transferring; }
private:
    Emulator* emu;
    bool transferring = false;
    uint16_t currentAddress = 0;

};


struct LCD
{
    // Bits to access the certain parts of the status register
    enum Status : uint8_t {
        PPUMODE = 0b11,
        LYC_LY = 0b100,
        MODE0 = 0b1000,
        MODE1 = 0b10000,
        MODE2 = 0b100000,
        LYC = 0b1000000

    };

    enum Control : uint8_t {
        BG_WINDOW_ENABLE = 0b1,
        OBJ_ENABLE = 0b10,
        OBJ_SIZE = 0b100,
        BG_TILEMAP = 0b1000,
        BG_WINDOW_TILES = 0b10000,
        WINDOW_ENABLE = 0b100000,
        WINDOW_TILEMAP = 0b1000000,
        LCD_PPU_ENABLE = 0b10000000
    };

    LCD();

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);
    
    void ConnectToEmulator(Emulator* emu);
    
    void Reset();

    
    bool GetStatusBit(const Status statusType) const noexcept { return !!(status & statusType); }
    bool GetControlBit(const Control controlType) const noexcept { return !!(lcdc & controlType); }

    void SetStatusBit(const Status statusType, uint8_t set);


    Emulator* emu;

    uint8_t lcdc;
    uint8_t ly;
    uint8_t lyc;
    uint8_t status;
    
    uint8_t scrollY;
    uint8_t scrollX;
    uint8_t windowY;
    uint8_t windowX;

    uint8_t bgp;
    uint8_t obp0;
    uint8_t obp1;

    uint8_t GetColor(uint8_t index, uint8_t pallete);

};  

struct OBJPixel
{
    bool exists = false; // whether a sprite pixels exists in the array or not
    uint8_t color;
    uint8_t pallete;
    //uint8_t sprite_priority; // for CGB only
    uint8_t background_priority;

};


class PPU
{

public:
    PPU();

    void OAM_write(uint16_t address, uint8_t data);
    uint8_t OAM_read(uint16_t address);

    void VRAM_write(uint16_t address, uint8_t data);
    uint8_t VRAM_read(uint16_t address);
    
    void tick();
    void Reset();

    void ConnectCPU(CPU* cpu);
    void ConnectLCD(LCD* lcd);
    void ConnectToEmulator(Emulator* emu);

    enum Mode {
        HBLANK,
        VBLANK,
        OAMSCAN,
        DRAWPIXELS
    };

    uint32_t videoBuffer[RESX * RESY];

    uint8_t windowLineCounter = 0;


private:

    CPU* cpu;
    LCD* lcd = nullptr;
    Emulator* emu;
    
    OAMEntry oam_ram[40];
    uint8_t vram[0x2000];

    Mode mode = OAMSCAN;
    void SwitchMode(Mode mode);

    void IncrementLY();


    uint16_t dots = 0;
    uint16_t scanline = 0; // this should be the same as ly register?
	uint16_t scanlineX = 0; // this is the x position in the scanline, used for pixel drawing
    uint16_t pushedX = 0;


    std::vector<Sprite> sprite_buffer;

    void HandleModeOAMScan();
    void HandleModeHBLANK();
    void HandleModeVBLANK();
    void HandleModeDrawPixels();

    // Pixel FIFO
    std::queue<uint8_t> background_pixels;
    std::array<OBJPixel, 160> sprite_pixels;

    uint8_t fetchedX = 0;
    uint16_t tileAddress;
    uint8_t tileLo;
    uint8_t tileHi;

    bool windowTriggered = false;


    enum FetchState {
        GetTile,
        DataLow,
        DataHigh,
        Sleep,
        Push
    };

    FetchState fetch_state = GetTile;

    void FetchSpritePixels();
    void FetchBackGroundPixels();
    void PixelRender();

    bool WindowVisible();
    void FetchWindowPixels();
};

