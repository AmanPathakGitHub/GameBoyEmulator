#pragma once

#include <array>
#include <string>
#include <vector>
#include <cstdint>

#include "mbc.h"

struct CartridgeHeader
{
								// Memory addresses
	uint8_t entrypoint[4];		// 0x100 - 0x103
	uint8_t nintendoLogo[48];	// 0x104 - 0x133
	char title[15]; 			// 0x134 - 0x142
	bool cgbFlag;				// 0x143
	char newLicenseeCode[2];	// 0x144 - 0x145
	bool sgbFlag;				// 0x146
	uint8_t cartridgeType;		// 0x147
	uint8_t romSize;			// 0x148
	uint8_t ramSize;			// 0x149
	uint8_t destinationCode;	// 0x14A
	uint8_t oldLicenseeCode;	// 0x14B
	uint8_t romVersion;			// 0x14C
	uint8_t headerCheckSum;		// 0x14D
	uint8_t globalCheckSum[2];	// 0x14E - 0x14F
};

class Cartridge
{
public:
	Cartridge(const std::string& filename);

	uint8_t ReadCart(uint16_t address);
	void WriteCart(uint16_t address, uint8_t data);

private:
	CartridgeHeader header;
	std::unique_ptr<MBC> memoryBankController;
	uint8_t* m_CartData; // all cart memory including whats in the different banks
	uint32_t m_ROM_size; // size in bytes

};