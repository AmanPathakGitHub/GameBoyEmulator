#pragma once
#include <cstdint>
#include <emulator.h>

#include <raylib.h>
#include <cstdint>


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

private:
	Emulator emu;

	RenderTexture2D renderTexture;

	uint32_t m_ScreenWidth, m_ScreenHeight;
	const char* m_TitleName;
		
	void DrawRegisters();
	void DrawMemory(uint32_t x, uint32_t y, uint16_t startAddress, uint16_t endAddress);

};