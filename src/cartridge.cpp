#include "cartridge.h"

#include <fstream>

Cartridge::Cartridge(const std::string& filename)
{
	std::ifstream ifs(filename, std::ios::binary);

	ifs.seekg(0x148); // offset where rom byte is
	uint8_t rom_size = 0;
	ifs.read((char*)&rom_size, 1);
	
	rom_size = 32768 * (1 << rom_size);
	
	m_CartData = (uint8_t*)malloc(rom_size);

	ifs.seekg(0);
	ifs.read((char*)m_CartData, rom_size);

	ifs.close();
}

uint8_t Cartridge::ReadCart(uint16_t address)
{
	//if(address < 0x4000)
	//	return m_CartData[address];
	//else if (address < 0x8000)
	//{
	//	// memory banking
	//	return m_CartData[address];
	//}

	return 0;

}

void Cartridge::WriteCart(uint16_t address, uint8_t data)
{
}
