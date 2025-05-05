#pragma once

#include <array>
#include <string>

class Cartridge
{
public:
	Cartridge(const std::string& filename);

	uint8_t ReadCart(uint16_t address);
	void WriteCart(uint16_t address, uint8_t data);

private:

	uint8_t* m_CartData; // all cart memory including whats in the different banks
	uint8_t  m_MemoryBank = 0;

};