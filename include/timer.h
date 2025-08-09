#pragma once

#include <cstdint>

#include "cpu.h"

class Timer
{
public:
    Timer();

    void tick();
    void ConnectTimerToCPU(CPU* cpu);

    void Reset();

    uint8_t read(uint16_t address);
    void write(uint16_t address, uint8_t data);
private:
    CPU* cpu; // for requesting interrupts
    uint16_t div;
    uint8_t tima;
    uint8_t tma;
    uint8_t tac;
};