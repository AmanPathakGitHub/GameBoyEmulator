#pragma once

#include <cstdint>
#include <array>
#include <memory>
#include <vector>
#include <string>

struct CartridgeHeader; // forward declare to avoid circular definition

// Memory bank controller
class MBC {
public:
	MBC(uint8_t* cartData);
	virtual ~MBC() = default;

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
	~MBC1() override;

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;
private:

	void save();

	uint8_t* externalRAM;

	bool ramEnabled = false;
	uint8_t romBankNumber = 1;
	uint8_t ramBankNumber = 0;
	uint8_t bankingMode = 0;

	size_t cartDataSize;
	size_t externalRAMSize;

	std::string title; // file name to save and read from
	
	bool requiresSave;
};

class MBC2 : public MBC
{
public:
	MBC2(uint8_t* cartData, const CartridgeHeader& header);
	~MBC2();
	

	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;

private:
	int romBankNumber = 1;

	std::array<uint8_t, 512> ram;
	bool requiresSave;

	bool ramEnabled = false;

	std::string title;

	void save();

};

// Real Time Clock (realised all only gameboy color games uses this which this emualtor does not support)
// basically this struct is useless
// keeping it here in case i want to ever make this a gameboy color emulator
struct RTC
{

	RTC() = default;

	uint8_t S;
	uint8_t M;
	uint8_t H;
	uint8_t DL;
	uint8_t DH;
	
	uint8_t latchRegister;

	void SetActive(uint8_t reg);
	void WriteActive(uint8_t data);
	uint8_t GetActive();
	void LatchRegisters(uint8_t data);
	bool IsHalted();
	uint8_t GetDayCounterCarry();
	
private:
	// shouldnt be able to set the pointer to whatever
	uint8_t* active = nullptr;

};

class MBC3 : public MBC
{
public:
	MBC3(uint8_t* cartData, const CartridgeHeader header, bool requiresSave);
	~MBC3();


	uint8_t read(uint16_t address) override;
	void write(uint16_t address, uint8_t data) override;


private:

	uint8_t romBankNumber = 1;
	uint8_t ramBankNumber = 1;

	uint8_t registerSelect = 0;

	bool ramAndTimerEnable = false;

	uint8_t* ram;
	RTC rtc;

	bool requiresSave;
	std::string title;
	uint32_t ramSize;

	void save();

};

std::unique_ptr<MBC> CreateMBCByType(const CartridgeHeader& header, uint8_t* cartData);