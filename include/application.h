#pragma once
#include <cstdint>
#include "emulator.h"

#include <fstream>

struct GLFWwindow;

#include <raylib.h>

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

	bool emu_run = false;


private:
	

	Emulator emu;

	RenderTexture2D renderTexture;
	RenderTexture2D tileMapTexture;
	RenderTexture2D tileIndexTexture;

	uint32_t m_ScreenWidth, m_ScreenHeight;
	const char* m_TitleName;

	void ImGuiDraw();
	void HandleInput();

	void display_tile(RenderTexture2D texture, uint16_t startLocation, uint16_t tileNum, int x, int y);

	bool showDisassembly = false;
	bool showMemoryView = false;
	bool showTileMap = false;
	bool showTileIndexMap = false;

	int tileIndexBaseAddress = 0x8000;

	void ShowMemoryViewText();
	void ShowDisassemblyText();
	void ShowTileIndexMap();


	static void WriteParams(Emulator& emu, CPU::Operand& op, std::stringstream& ss, uint16_t& currentAddress);

};

