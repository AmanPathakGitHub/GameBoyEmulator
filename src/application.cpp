#include "application.h"

#include <format>
#include <iostream>
#include <string>
#include <sstream>

#include <imgui.h>
#include <rlImGui.h>
#include <imgui_impl_raylib.h>
#include <print>
#include <filesystem>

#include "platformUtils.h"
#include "panels/memoryview.h"
#include "panels/disassembler.h"
#include "panels/tileviewer.h"
#include "panels/mapviewer.h"

Application::Application(const ApplicationSettings settings)
	: m_ScreenWidth(settings.screenWidth), m_ScreenHeight(settings.screenHeight), m_TitleName(settings.TitleName)
{

	if (settings.resizable)
		SetConfigFlags(FLAG_WINDOW_RESIZABLE | FLAG_MSAA_4X_HINT);

	InitWindow(settings.screenWidth, settings.screenHeight, settings.TitleName);

	///*
	//	Different FPS?
	//	the amount of clock cycles the gameboy can exectue every second is 4194304
	//	which means that if each frame we update the emulator 60 times a second the each frame will execute 69905(4194304/60) clock cycles.
	//	This will ensure the emulator is run at the correct speed.

	//	maybe application can run at different fps to the emulator, e.g application runs at 120fps while emulation is at 60
	//	right now both are locked to 60
	//*/
	SetTargetFPS(60); 

	renderTexture = LoadRenderTexture(RESX, RESY);

	m_Panels.reserve(4); // total of 4 maximum panels

	startupPath = std::filesystem::current_path();



	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	io.ConfigViewportsNoAutoMerge = false;  // Allow separate viewports to merge
	io.ConfigViewportsNoTaskBarIcon = false;  // Show floating ImGui windows in the taskbar

	static std::string iniPath = (startupPath / "imgui.ini").string(); // Pointer would be dangling if it was not static
	io.IniFilename = iniPath.c_str();

	ImGui::StyleColorsClassic();

	ImGuiStyle& style = ImGui::GetStyle();
	if (io.ConfigFlags & ImGuiConfigFlags_ViewportsEnable)
	{
		style.WindowRounding = 0.0f;
		style.Colors[ImGuiCol_WindowBg].w = 1.0f;
	}

	ImGui_ImplRaylib_Init();

}

Application::~Application()
{

	std::filesystem::current_path(startupPath);

	ImGui_ImplRaylib_Shutdown();
	ImGui::DestroyContext();

	CloseWindow();

}


template<PanelType T>
void Application::CreatePanel(T* panel)
{
	// omg this line looks horrible, need to overload the comparison operator and maybe an id system? comparing names works just fine
	if (std::find_if(m_Panels.begin(), m_Panels.end(), [&panel](std::unique_ptr<Panel>& p) { return p->m_Name == panel->m_Name; }) != m_Panels.end())
		return;

	m_Panels.emplace_back(panel);
}

void Application::ImGuiDraw()
{

	ImGui::Begin("Emulator");
	/*	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);*/
	ImVec2 windowSize = ImGui::GetContentRegionAvail();

	float scale = std::min(windowSize.x / renderTexture.texture.width, windowSize.y / renderTexture.texture.height);

	ImVec2 scaledSize = ImVec2(renderTexture.texture.width * scale, renderTexture.texture.height * scale);

	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (windowSize.x - scaledSize.x) / 2, ImGui::GetCursorPosY() + (windowSize.y - scaledSize.y) / 2));

	ImGui::Image((ImTextureID)renderTexture.texture.id, scaledSize, ImVec2(0, 1), ImVec2(1, 0));

	if (ImGui::IsWindowFocused())
		HandleInput();

	ImGui::End();


	if (ImGui::BeginMainMenuBar()) {
		if (ImGui::BeginMenu("File")) {
			if (ImGui::MenuItem("Open")) {
				std::string filePath = Utils::OpenFileDialog(GetWindowHandle(), ".gb\0");
				if (!filePath.empty())
				{

					try 
					{
						emu.LoadROM(filePath);
						std::cout << "Loaded: " << filePath << std::endl;
						emu_run = true;
						emu.Reset();
					}
					catch (std::exception e)
					{
						Utils::ShowMessageBox(GetWindowHandle(), e.what(), "Error");
					}
					
				}
	
				
			}

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Debug")) {
			if (ImGui::MenuItem("Memory View"))
				CreatePanel<MemoryView>(new MemoryView{ emu, "Memory View" });
			if (ImGui::MenuItem("Disassembly"))
				CreatePanel<Disassembler>(new Disassembler{ emu, "Disassembly" });
			if (ImGui::MenuItem("TileMap"))
				CreatePanel<TileViewer>(new TileViewer{ emu });
			if (ImGui::MenuItem("Map Viewer"))
				CreatePanel<MapViewer>(new MapViewer{ emu });
			if (ImGui::MenuItem("Toggle FPS"))
				m_ShowFPS = !m_ShowFPS;

			ImGui::EndMenu();
		}

		if (ImGui::BeginMenu("Emulation"))
		{
			if (ImGui::MenuItem(emu_run ? "Pause" : "Continue"))
				emu_run = !emu_run;
			if (ImGui::BeginMenu("Speed"))
			{
				if (ImGui::MenuItem("100%")) SetTargetFPS(60);
				if (ImGui::MenuItem("200%")) SetTargetFPS(60 * 2);
				if (ImGui::MenuItem("300%")) SetTargetFPS(60 * 3);
				if (ImGui::MenuItem("400%")) SetTargetFPS(60 * 4);
				if (ImGui::MenuItem("Max Speed")) SetTargetFPS(-1);
				
				ImGui::EndMenu();
			}

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
	

}

std::array<uint32_t, RESX * RESY> Application::GetVideoBuffer()
{
	std::array<uint32_t, RESX* RESY> outputArray;
	int index = 0;
	for (int y = RESY - 1; y >= 0; y--)
	{
		for (int x = 0; x < RESX; x++)
		{
			outputArray[index++] = emu.ppu.videoBuffer[y * RESX + x];
		}
	}

	return outputArray;
}

void Application::HandleInput()
{
	// INPUT

	ImGuiIO& io = ImGui::GetIO();


	if (IsKeyPressed(KEY_UP))
		emu.buttonState.up = true;
	if (IsKeyPressed(KEY_DOWN))
		emu.buttonState.down = true;
	if (IsKeyPressed(KEY_LEFT))
		emu.buttonState.left = true;
	if (IsKeyPressed(KEY_RIGHT))
		emu.buttonState.right = true;
	if (IsKeyPressed(KEY_Z))
		emu.buttonState.b = true;
	if (IsKeyPressed(KEY_X))
		emu.buttonState.a = true;
	if (IsKeyPressed(KEY_ENTER))
		emu.buttonState.start = true;
	if (IsKeyPressed(KEY_RIGHT_SHIFT))
		emu.buttonState.select = true;

	if (IsKeyReleased(KEY_UP))
		emu.buttonState.up = false;
	if (IsKeyReleased(KEY_DOWN))
		emu.buttonState.down = false;
	if (IsKeyReleased(KEY_LEFT))
		emu.buttonState.left = false;
	if (IsKeyReleased(KEY_RIGHT))
		emu.buttonState.right = false;
	if (IsKeyReleased(KEY_Z))
		emu.buttonState.b = false;
	if (IsKeyReleased(KEY_X))
		emu.buttonState.a = false;
	if (IsKeyReleased(KEY_ENTER))
		emu.buttonState.start = false;
	if (IsKeyReleased(KEY_RIGHT_SHIFT))
		emu.buttonState.select = false;

	if (GetKeyPressed() != 0)
	{
		emu.cpu.RequestInterrupt(CPU::JOYPAD);
	}


}


void Application::Run()
{
	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		ImGui_ImplRaylib_ProcessEvents();

		// Start the Dear ImGui frame
		ImGui_ImplRaylib_NewFrame();
		ImGui::NewFrame();

		ImGui::DockSpaceOverViewport();

		// DEBUGING INPUT KEYS NOT FOR GAMEPLAY
		if (IsKeyPressed(KEY_C))
		{
			emu.clock();
		}
		else if (IsKeyPressed(KeyboardKey(KEY_R)))
		{
			emu.cpu.Reset();
		}
		else if (IsKeyPressed(KEY_SPACE))
		{
			emu_run = !emu_run;
		}
		else if (IsKeyPressed(KEY_P))
		{
			for (int i = 0; i < 100; i++)
				emu.clock();
		}
		else if (IsKeyPressed(KEY_K))
		{
			for (int i = 0; i < 20; i++)
				emu.clock();
		}


		if (emu_run && emu.romLoaded) emu.UpdateFrame();
		ImGuiDraw();

		UpdateTexture(renderTexture.texture, GetVideoBuffer().data());

		BeginTextureMode(renderTexture);
		
		if (!emu.romLoaded)
			DrawText("No rom loaded", RESX / 2 - (14 * 5 / 2), RESY / 2 - 5, 10, WHITE);

		if (!emu_run && emu.romLoaded)
			DrawText("Paused", 1, 1, 10, RED);
		if (m_ShowFPS)
			DrawText(std::format("{}", GetFPS()).c_str(), 10, 130, 10, GREEN);
		

 		EndTextureMode();

		for (std::unique_ptr<Panel>& p : m_Panels)
		{
			if (!p->m_IsVisible) continue;

			ImGui::Begin(p->m_Name.c_str(), &p->m_IsVisible);
			p->Update();
			ImGui::End();
		}

		m_Panels.erase(std::remove_if(m_Panels.begin(), m_Panels.end(), [](const std::unique_ptr<Panel>& p) { return !p->m_IsVisible; }), m_Panels.end());

			
		ImGui::Render();
		BeginDrawing();
		ImGui_ImplRaylib_RenderDrawData(ImGui::GetDrawData());
		EndDrawing();


	}

}
