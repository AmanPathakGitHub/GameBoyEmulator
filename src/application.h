#pragma once
#include <cstdint>
#include "emulator.h"
#include "panels/panel.h"

#include <fstream>
#include <filesystem>

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


	bool emu_run = false;


private:
	

	Emulator emu;

	RenderTexture2D renderTexture;

	uint32_t m_ScreenWidth, m_ScreenHeight;
	const char* m_TitleName;

	std::vector<std::unique_ptr<Panel>> m_Panels;
	bool m_ShowFPS = false;

	std::filesystem::path startupPath; // used to make sure imgui.ini file is saved the correct location


	template<PanelType T>
	void CreatePanel(T* panel);

	void ImGuiDraw();
	void HandleInput();
	
	std::array<uint32_t, RESX* RESY> GetVideoBuffer();

};

