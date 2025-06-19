#include "timer.h"

Timer::Timer()
{
    div = 0xAC00;
}

void Timer::ConnectTimerToCPU(CPU* cpu)
{
    this->cpu = cpu;
    div = 0xABCC; // happens when cpu resets, probably should go somewhere else

}


void Timer::tick()
{
    uint16_t prev_div = div;

    div++;

    bool timer_update = false;

    switch (tac & 0b11)
    {
    case 0b00:
        timer_update = (prev_div & (1 << 9)) && (!(div & (1 << 9)));
        break;
    
    case 0b01:
        timer_update = (prev_div & (1 << 3)) && (!(div & (1 << 3)));
        break;
    
    case 0b10:
        timer_update = (prev_div & (1 << 5)) && (!(div & (1 << 5)));
        break;

    case 0b11:
        timer_update = (prev_div & (1 << 7)) && (!(div & (1 << 7)));
        break;

    }

    if(timer_update && tac & (1 << 2)) {
        tima++;

        if(tima == 0xFF)
        {
            tima = tma;
            // request interrupt
            cpu->RequestInterrupt(INT_TIMER);
        }
            
    }
}

uint8_t Timer::read(uint16_t address)
{
    switch (address)
    {
    case 0xFF04:
        return div >> 8;
    case 0xFF05:
        return tima;
    case 0xFF06:
        return tma;
    case 0xFF07:
        return tac;
    
    default:
        break;
    }
    return 0;
}

void Timer::write(uint16_t address, uint8_t data)
{
    switch (address)
    {
    case 0xFF04:
        div = 0;
        break;
    case 0xFF05:
        tima = data;
        break;
    case 0xFF06:
        tma = data;
        break;
    case 0xFF07:
        tac = data;
        break;
    default:
        break;
    }
}