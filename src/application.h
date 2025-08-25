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

	static std::vector<std::string> disassemble(Emulator& emu, uint16_t startAddress, uint16_t endAddress);

	bool emu_run = false;


private:
	

	Emulator emu;

	RenderTexture2D renderTexture;
	RenderTexture2D tileMapTexture;
	RenderTexture2D tileIndexTexture;
	Texture2D gameBoyOutput;
	Image img;

	uint32_t m_ScreenWidth, m_ScreenHeight;
	const char* m_TitleName;

	std::string m_ErrorMessage;

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

	void ShowError(const std::string& errorMessage);


	static void WriteParams(Emulator& emu, CPU::Operand& op, std::stringstream& ss, uint16_t& currentAddress);

};

