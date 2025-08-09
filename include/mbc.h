#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <vector>

struct CartridgeHeader; // forward declare to avoid circular definition

// Memory bank controller
class MBC {
public:
	MBC(uint8_t* cartData);

	virtual uint8_t read(uint16_t address) = 0;
	virtual void write(uint16_t address, uint8_t data) = 0;
protected:
	uint8_t* cartData;
};
// ROM ONLY
class MBC0 : public MBC {
public:
	MBC0(uint8_t* cartData);

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;

private:
	std::array<uint8_t, 0x2000> externalRam;
};

class MBC1 : public MBC {
public:
	MBC1(uint8_t* cartData, const CartridgeHeader& header);
	~MBC1();

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;
private:
	bool ramEnabled = false;
	uint8_t romBankNumber = 1;
	uint8_t ramBankNumber = 0;
	uint8_t bankingMode = 0;

	size_t cartDataSize;
	
	uint8_t* externalRAM;
};

std::unique_ptr<MBC> CreateMBCByType(const CartridgeHeader& header, uint8_t* cartData);