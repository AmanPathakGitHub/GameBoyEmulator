#pragma once
#include <cstdint>
#include <emulator.h>

#include <raylib.h>

#include <fstream>

struct ApplicationSettings
{
	const char* TitleName;
	uint32_t screenWidth, screenHeight;
	bool resizable = false;
};

class Application
{
public:
	Application(const ApplicationSettings settings);
	~Application();

	void Run();

	static std::vector<std::string> dissassemble(Emulator& emu, uint16_t startAddress, uint16_t endAddress);
	static std::string RegTypeToString(CPU::RegType reg);
	static std::string CondTypeToString(CPU::CondType cond);

private:
	Emulator emu;
	bool emu_run = false;

	RenderTexture2D renderTexture;
	RenderTexture2D tileMapTexture;

	uint32_t m_ScreenWidth, m_ScreenHeight;
	const char* m_TitleName;

	void DrawTiles();
	void display_tile(RenderTexture2D texture, uint16_t startLocation, uint16_t tileNum, int x, int y);

		
	void DrawRegisters();
	void DrawMemory(uint32_t x, uint32_t y, uint16_t startAddress, uint16_t endAddress);

	static void WriteParams(Emulator& emu, CPU::Operand& op, std::stringstream& ss, uint16_t& currentAddress);


};