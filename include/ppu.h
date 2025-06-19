#pragma once

#include <cstdint>

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

struct DMA
{
    uint16_t currentAddress = 0;
    Emulator* emu;
    void ConnectToEmulator(Emulator* emu);
    void StartTransfer(uint8_t value);
    void Tick();
    inline bool isTransferring() { return transferring; }
private:
    bool transferring = false;

};

class PPU
{

public:
    void OAM_write(uint16_t address, uint8_t data);
    uint8_t OAM_read(uint16_t address);

    void VRAM_write(uint16_t address, uint8_t data);
    uint8_t VRAM_read(uint16_t address);


private:
    OAMEntry oam_ram[40];
    uint8_t vram[0x2000];

    
};

