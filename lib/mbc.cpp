#include "mbc.h"
#include "cartridge.h"

#include <cassert>
#include <stdexcept>
#include <cstring>
#include <iostream>
#include <expected>
#include <chrono>
#include <fstream>

#include <filesystem>


static uint32_t GetRAMSize(uint8_t ramByte)
{
	switch (ramByte)
	{
	case 0: return 0;
	case 2: return 8 * 1024;
	case 3: return 32 * 1024;
	case 4: return 128 * 1024;
	case 5: return 64 * 1024;
	}
}


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
	if (address >= 0xA000)
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


	std::string fileName = title + ".sav";

	if (std::filesystem::exists(fileName))
	{
		std::ifstream ifs(fileName);

		ifs.read((char*)externalRAM, ramSize);
	}
	else
		memset((char*)externalRAM, 0, ramSize);


	if (requiresSave)
		ofs.open(fileName);


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


	if (!ofs.is_open()) throw std::runtime_error("Save file could not be open");

	ofs.write((char*)externalRAM, externalRAMSize);

}


MBC2::MBC2(uint8_t* cartData, const CartridgeHeader& header)
	: MBC(cartData), ram({})
{
	requiresSave = header.cartridgeType == 6;
	title = header.title;



	std::string fileName = title + ".sav";

	if (std::filesystem::exists(fileName))
	{
		std::ifstream ifs(fileName);

		ifs.read((char*)ram.data(), ram.size());
	}
	else
		memset((char*)ram.data(), 0, ram.size());


	if (requiresSave)
		ofs.open(fileName);


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

	if (!ofs.is_open()) throw std::runtime_error("Save file could not be open");

	ofs.write((char*)ram.data(), ram.size());

}


void RTC::SetActive(uint8_t reg)
{
	switch (reg)
	{
	case 0x08: active = &S; break;
	case 0x09: active = &M; break;
	case 0x0A: active = &H; break;
	case 0x0B: active = &DL; break;
	case 0x0C: active = &DH; break;
	//default: active = nullptr;
	}
}

void RTC::WriteActive(uint8_t data)
{
	if (IsHalted()) return;
	*active = data;
}

uint8_t RTC::GetActive()
{
	return *active;
}

void RTC::LatchRegisters(uint8_t data)
{
	if (latchRegister == 0 && data == 1)
	{
		using namespace std::chrono;
		auto currentTime = std::chrono::system_clock::now();

		S = time_point_cast<seconds>(currentTime).time_since_epoch().count() % 60;
		M = time_point_cast<minutes>(currentTime).time_since_epoch().count() % 60;
		H = time_point_cast<hours>(currentTime).time_since_epoch().count() % 24;

		uint16_t dayCounter = time_point_cast<days>(currentTime).time_since_epoch().count() % 512;

		DL = dayCounter & 0xFF;
		uint8_t dhUpperBit = !!(dayCounter & (1 << 8));
		
		if (dhUpperBit) DH |= 1;
		else DH &= ~(1);

		if (dayCounter == 0) // wrapped from 511 -> 0
			DH |= (1 << 7);
	}

	latchRegister = data;
	
}

bool RTC::IsHalted()
{
	return !!(DH & (1 << 6));
}

uint8_t RTC::GetDayCounterCarry()
{
	return DH & (1 << 7);
}


MBC3::MBC3(uint8_t* cartData, const CartridgeHeader header, bool requiresSave = false)
	: MBC(cartData), requiresSave(requiresSave)
{
	ramSize = GetRAMSize(header.ramSize);
	ram = new uint8_t[ramSize];

	title = header.title;
	
	std::string fileName = title + ".sav";


	if (std::filesystem::exists(fileName))
	{
		std::ifstream ifs(fileName);

		ifs.read((char*)ram, ramSize);
	}
	else
		memset((char*)ram, 0, ramSize);


	if (requiresSave)
		ofs.open(fileName);

		
}

MBC3::~MBC3()
{
	save();
	delete[] ram;
}

void MBC3::save()
{

	if (!requiresSave) return;

	if (!ofs.is_open()) throw std::runtime_error("Save file could not be open");
	ofs.seekp(0);
	ofs.write((char*)ram, ramSize);

	if (!ofs.good()) throw std::runtime_error("Failed to write");
}


uint8_t MBC3::read(uint16_t address)
{
	if (address < 0x4000)
		return cartData[address];
	else if (address < 0x8000)
	{
		int effectiveAddress = (address - 0x4000) + romBankNumber * 0x4000;
		return cartData[effectiveAddress];
	}
	else if (address >= 0xA000 && address <= 0xBFFF)
	{
		if (!ramAndTimerEnable) return 0xFF;

		if (registerSelect < 8)
		{
			int effectiveAddress = (address - 0xA000) + ramBankNumber * 0x2000;
			return ram[effectiveAddress];
		}

		return rtc.GetActive();
	}

	return 0xFF;
}

void MBC3::write(uint16_t address, uint8_t data)
{
	if (address < 0x2000)
		ramAndTimerEnable = (data & 0xF) == 0xA;
	else if (address < 0x4000)
	{
		romBankNumber = data & 0x7F;
		if (romBankNumber == 0) romBankNumber = 1;
	}
	else if (address < 0x6000)
	{
		registerSelect = data & 0xF;
		if (registerSelect < 8)
		{
			ramBankNumber = data & 0xF;
			return;
		}

		rtc.SetActive(registerSelect);
		
	}
	else if (address < 0x8000)
	{
		rtc.LatchRegisters(data);
	}
	else if (address >= 0xA000 && address <= 0xC000)
	{
		if (!ramAndTimerEnable) return;

		if (registerSelect < 8)
		{
			int effectiveAddress = (address - 0xA000) + ramBankNumber * 0x2000;
			ram[effectiveAddress] = data;
			save();
			return;
		}

		rtc.WriteActive(data);
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
	case 5: return std::make_unique<MBC2>(cartData, header);
	case 6: return std::make_unique<MBC2>(cartData, header);
	case 0xF: return std::make_unique<MBC3>(cartData, header, true);
	case 0x10: return std::make_unique<MBC3>(cartData, header, true);
	case 0x11: return std::make_unique<MBC3>(cartData, header);
	case 0x12: return std::make_unique<MBC3>(cartData, header);
	case 0x13: return std::make_unique<MBC3>(cartData, header, true);
	default: throw std::runtime_error("ROM TYPE NOT SUPPORTED");
	}
}

