#include "mbc.h"
#include "cartridge.h"

#include <cassert>
#include <stdexcept>

#include <iostream>

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
	memset(externalRAM, 0, ramSize);

	//if(ramSize != 0) externalRAM.reserve(ramSize);

	//for (int i = 0; i < ramSize; i++)
	//	externalRAM.emplace_back(0);

	cartDataSize = 32768 * (1 << header.romSize);
	
}

MBC1::~MBC1()
{
	delete[] externalRAM;
}

uint8_t MBC1::read(uint16_t address)
{
	if (address < 0x4000)
		return cartData[address];
	
	if ((address & 0xE000) == 0xA000)
	{
		if (!ramEnabled)
			return 0xFF;

		//assert((address - 0xA000) * ramBankNumber * 0x2000 < externalRAM.size());
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
	
	if ((address & 0xE000) == 0x2000)
	{
		if (data == 0) data = 1;
		data &= 0b11111;
		romBankNumber = data;
	}

	if ((address & 0xE000) == 0x4000)
	{
		ramBankNumber = data & 0b11;
	}

	if ((address & 0xE000) == 0x6000)
	{
		bankingMode = data & 0b1;

	}

	if ((address & 0xE000) == 0xA000)
	{
		if (ramEnabled)
		{
			externalRAM[address - 0xA000] = data;
		}
	}
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
	default: throw std::runtime_error("CARTRIDGE TYPE NOT SUPPORTED"); // NOT SUPPORTED
	}
}

