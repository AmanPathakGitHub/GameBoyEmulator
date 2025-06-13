#include "cartridge.h"

#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string& filename)
{
	std::ifstream ifs(filename, std::ios::binary);

	ifs.seekg(0x148); // offset where rom byte is
	uint8_t rom_byte = 0;
	ifs.read((char*)&rom_byte, 1);
	
	std::cout << rom_byte << std::endl;
	
	m_ROM_size = 32768 * (1 << rom_byte);
	
	m_CartData = (uint8_t*)malloc(m_ROM_size);

	ifs.seekg(0);
	ifs.read((char*)m_CartData, m_ROM_size);

	ifs.close();

	std::cout << "ROM SIZE: " << (int)m_ROM_size << std::endl;
}

uint8_t Cartridge::ReadCart(uint16_t address)
{

	if(address < 0x4000)
		return m_CartData[address];
	else if (address < 0x8000)
	{
		// memory banking
		return m_CartData[address];
	}



	return 0;

}

void Cartridge::WriteCart(uint16_t address, uint8_t data)
{

	m_CartData[address] = data;
}
