#pragma once
#include <cstdint>
#include "emulator.h"
#include "panels/panel.h"

#include <fstream>

struct GLFWwindow;

#include <raylib.h>

template<typename T>
concept PanelType = std::is_base_of_v<Panel, T>;

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

	Texture2D gameBoyOutput;
	Image img;

	uint32_t m_ScreenWidth, m_ScreenHeight;
	const char* m_TitleName;

	std::string m_ErrorMessage;

	std::vector<std::unique_ptr<Panel>> m_Panels;


	template<PanelType T>
	void CreatePanel(T* panel);

	void ImGuiDraw();
	void HandleInput();

	void ShowError(const std::string& errorMessage);


	static void WriteParams(Emulator& emu, CPU::Operand& op, std::stringstream& ss, uint16_t& currentAddress);

};

