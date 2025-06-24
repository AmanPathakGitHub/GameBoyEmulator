#pragma once

#include <cstdint>
#include <queue>

struct OAMEntry
{
    uint8_t y;
    uint8_t x;
    uint8_t tile_index;

    unsigned f_palette : 3;
    unsigned f_bank : 1;
    unsigned f_dmg_pallete : 1;
    unsigned f_xflip : 1;
    unsigned f_yflip: 1;
    unsigned f_priority : 1;
};

class Emulator;

class DMA
{
public:
    void ConnectToEmulator(Emulator* emu);
    void StartTransfer(uint8_t value);
    void Tick();
    inline bool isTransferring() { return transferring; }
private:
    Emulator* emu;
    bool transferring = false;
    uint16_t currentAddress = 0;

};


// maybe use enums for this
#define LCD_PPUMODE 0b11
#define LCD_LYC_LY 0b100
#define LCD_MODE0 0b1000
#define LCD_MODE1 0b10000
#define LCD_MODE2 0b100000
#define LCD_LYC 0b1000000

#define LCDC_BG_WINDOW_ENABLE 0b1
#define LCDC_OBJ_ENABLE 0b10
#define LCDC_OBJ_SIZE 0b100
#define LCDC_BG_TILEMAP 0b1000
#define LCDC_BG_WINDOW_TILES 0b10000
#define LCDC_WINDOW_ENABLE 0b100000
#define LCDC_WINDOW_TILEMAP 0b1000000
#define LCDC_LCD_PPU_ENABLE 0b10000000

struct LCD
{
    LCD();

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);
    void ConnectToEmulator(Emulator* emu);
    inline bool GetStatusBit(uint8_t statusType) { return !!(status & statusType); }
    inline bool GetControlBit(uint8_t controlType) { return !!(lcdc & controlType); }

    Emulator* emu;

    uint8_t lcdc;
    uint8_t ly;
    uint8_t lyc;
    uint8_t status;
    
    uint8_t scrollY;
    uint8_t scrollX;
    uint8_t windowY;
    uint8_t windowX;

};  

struct Pixel
{
    uint8_t color;
    uint8_t pallete;
    uint8_t sprite_priority; // for CGB only
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

    void ConnectLCD(LCD* lcd);

    enum Mode {
        HBLANK,
        VBLANK,
        OAMSCAN,
        DRAWPIXELS
    };

private:

    LCD* lcd;
    
    OAMEntry oam_ram[40];
    uint8_t vram[0x2000];

    Mode mode = OAMSCAN;

    uint16_t dots = 0;
    uint16_t scanline = 0; // this should be the same as ly register?


    std::vector<OAMEntry> sprite_buffer; 

    void HandleModeOAMScan();
    void HandleModeHBLANK();
    void HandleModeVBLANK();
    void HandleModeDrawPixels();

    // Pixel FIFO
    std::queue<Pixel> background_pixels;
    std::queue<Pixel> sprite_pixels;

    uint16_t xPositionCounter = 0;

    enum FetchState {
        GetTile,
        DataLow,
        DataHigh,
        Sleep,
        Push
    };

    FetchState fetch_state = GetTile;

    void FetchBackGroundPixels();
    
};

