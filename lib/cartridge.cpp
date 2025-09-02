#include "cartridge.h"

#include <fstream>
#include <iostream>

Cartridge::Cartridge(const std::string& filename)
{
	std::ifstream ifs(filename, std::ios::binary);

	if(!ifs.good()) std::runtime_error("ERROR LOADING FILE");

	// 0x4F, is the size of the header
	ifs.seekg(0x100);
	ifs.read((char*)&m_Header, 0x4F);


	m_ROM_size = 32768 * (1 << m_Header.romSize);
	
	m_CartData = (uint8_t*)malloc(m_ROM_size);

	ifs.seekg(0);
	ifs.read((char*)m_CartData, m_ROM_size);

	ifs.close();


	m_MemoryBankController = CreateMBCByType(m_Header, m_CartData);

	std::cout << "ROM SIZE: " << (int)m_ROM_size << std::endl;
}



uint8_t Cartridge::ReadCart(uint16_t address)
{
	return m_MemoryBankController->read(address);
}

void Cartridge::WriteCart(uint16_t address, uint8_t data)
{
	m_MemoryBankController->write(address, data);
}
