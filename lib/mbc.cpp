#include "mbc.h"
#include "cartridge.h"

#include <cassert>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <expected>

#include <fstream>

MBC::MBC(uint8_t* cartData)
{
	this->cartData = cartData;
}

MBC0::MBC0(uint8_t* cartData)
	: MBC(cartData)
{
	memset(externalRam.data(), 0, 0x2000);
}

uint8_t MBC0::read(uint16_t address)
{
	if (address < 0x8000)
		return cartData[address];
	else
		return externalRam[address - 0xA000];
}

void MBC0::write(uint16_t address, uint8_t data)
{
	if (address < 0x8000)
		cartData[address] = data;
	else
		externalRam[address - 0xA000] = data;
}


MBC1::MBC1(uint8_t* cartData, const CartridgeHeader& header)
	: MBC(cartData)
{
	// MBC1 only supports up to 32KiB of external RAM
	assert(header.ramSize <= 3);

	size_t ramSize = 0;
	if (header.ramSize == 2) ramSize = 8 * 1024;
	else if (header.ramSize == 3) ramSize = 32 * 1024;

	externalRAM = new uint8_t[ramSize];
	externalRAMSize = ramSize;

	title = header.title;


	requiresSave = header.cartridgeType != 1;

	std::fstream fs;

	if (requiresSave) fs.open(title + ".sav");

	if (fs.is_open())
	{
		fs.read((char*)externalRAM, ramSize);
	}
	else 
		memset(externalRAM, 0, ramSize);


	cartDataSize = 32768 * (1 << header.romSize);
	
}

MBC1::~MBC1()
{
	save();
	delete[] externalRAM;
}

uint8_t MBC1::read(uint16_t address)
{
	if (address < 0x4000)
		return cartData[address];
	
	if ((address & 0xE000) == 0xA000)
	{
		if (!ramEnabled) return 0xFF;

		uint8_t ramBank = bankingMode == 0 ? 0 : ramBankNumber;
		int effectiveAddress = (address - 0xA000) + ramBank * 0x2000;

		return externalRAM[effectiveAddress];
	}

	int effectiveAddress1 = (address - 0x4000) + romBankNumber * 0x4000;

	assert(effectiveAddress1 < cartDataSize);

	return cartData[effectiveAddress1];
}

void MBC1::write(uint16_t address, uint8_t data)
{
	if (address < 0x2000)
		ramEnabled = data == 0xA;
	
	else if (address < 0x4000)
	{
		if (data == 0) data = 1;
		data &= 0b11111;
		romBankNumber = data;
	}

	else if ((address & 0xE000) == 0x4000)
	{
		ramBankNumber = data & 0b11;
	}

	else if ((address & 0xE000) == 0x6000)
	{
		bankingMode = data & 0b1;
	}

	else if ((address & 0xE000) == 0xA000)
	{
		if (ramEnabled)
		{
			uint8_t ramBank = bankingMode == 0 ? 0 : ramBankNumber;
			int effectiveAddress = (address - 0xA000) + ramBank * 0x2000;

			externalRAM[effectiveAddress] = data;
			save();
		}
	}
}

void MBC1::save()
{
	if (!requiresSave) return;

	std::ofstream fs(title + ".sav", std::ios::binary);

	if (!fs.is_open()) throw std::runtime_error("Save file could not be open");

	std::cout << externalRAMSize << std::endl;

	fs.write((char*)externalRAM, externalRAMSize);

	fs.close();
}


MBC2::MBC2(uint8_t* cartData, const CartridgeHeader& header)
	: MBC(cartData), ram({})
{
	requiresSave = header.cartridgeType == 6;
	title = header.title;


	std::fstream fs;

	if (requiresSave) fs.open(title + ".sav");

	if (fs.is_open())
	{
		fs.read((char*)ram.data(), ram.size());
	}
	else
		ram.fill(0);

}

MBC2::~MBC2()
{
	save();
}

uint8_t MBC2::read(uint16_t address)
{
	if (address < 0x4000)
		return cartData[address];
	else if (address < 0x8000)
	{
		int effectiveAddress = (address - 0x4000) + romBankNumber * 0x4000;
		assert(effectiveAddress < 262144);
		return cartData[effectiveAddress];
	}

	//////////////////////
	else if (address >= 0xA000 && address < 0xA200)
	{
		if (!ramEnabled) return 0xBB;
		return (ram[address - 0xA000] & 0xF) | 0xF0;
	}
	else if (address >= 0xA200 && address < 0xC000)
	{
		if (!ramEnabled) return 0xBB;
		return (ram[address & 0x1FF] & 0xF) | 0xF0; // lower 9 bits only
	}
	//////////////////////////
	__debugbreak();
}


void MBC2::write(uint16_t address, uint8_t data)
{

	if (address < 0x4000)
	{
		bool isBitSet = address & 0x100; // bit 8 of the address

		if (isBitSet == 0)
		{
			ramEnabled = (data & 0xF) == 0xA;
		}
		else
		{
			romBankNumber = data & 0xF;
			if (romBankNumber == 0) romBankNumber = 1;
		}
	}

	else if (address >= 0xA000 && address < 0xA200)
	{
		if (!ramEnabled) return;
		int effectiveAddress = address - 0xA000;
		ram[effectiveAddress] = data;
		save();
	}
	else if (address >= 0xA200 && address < 0xC000)
	{
		if (!ramEnabled) return;
		ram[address & 0x1FF] = data & 0xF;
		save();
	}

}

void MBC2::save()
{
	if (!requiresSave) return;

	std::ofstream fs(title + ".sav", std::ios::binary);

	if (!fs.is_open()) throw std::runtime_error("Save file could not be open");

	fs.write((char*)ram.data(), ram.size());

	fs.close();
}


std::unique_ptr<MBC> CreateMBCByType(const CartridgeHeader& header, uint8_t* cartData)
{
	std::cout << "Cart Type: " << (int)header.cartridgeType << std::endl;
	switch (header.cartridgeType)
	{
	case 0: return std::make_unique<MBC0>(cartData);
	case 1: return std::make_unique<MBC1>(cartData, header);
	case 2: return std::make_unique<MBC1>(cartData, header);
	case 3: return std::make_unique<MBC1>(cartData, header);
	case 5: return std::make_unique<MBC2>(cartData, header);
	case 6: return std::make_unique<MBC2>(cartData, header);
	default: throw std::runtime_error("ROM TYPE NOT SUPPORTED");
	}
}

