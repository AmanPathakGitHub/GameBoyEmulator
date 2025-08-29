#include "application.h"

#include <format>
#include <iostream>
#include <string>
#include <sstream>

#include <imgui.h>
#include <rlImGui.h>
#include <imgui_impl_raylib.h>
#include <print>

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
	//SetTargetFPS(60); 

	renderTexture = LoadRenderTexture(RESX, RESY);

	img = GenImageColor(RESX, RESY, GREEN);

	gameBoyOutput = LoadTextureFromImage(img);



	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;         // Enable Docking
	io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable;       // Enable Multi-Viewport / Platform Windows

	io.ConfigViewportsNoAutoMerge = false;  // Allow separate viewports to merge
	io.ConfigViewportsNoTaskBarIcon = false;  // Show floating ImGui windows in the taskbar

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

	ImGui_ImplRaylib_Shutdown();
	ImGui::DestroyContext();

	CloseWindow();

}

void Application::ShowError(const std::string& errorMessage)
{
	DrawText(errorMessage.c_str(), GetScreenWidth() / 2, GetScreenHeight() / 2, 10, RED);
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

	ImGui::Image((ImTextureID)renderTexture.texture.id, scaledSize);

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
			if (ImGui::MenuItem("TileMap Indexes"))
				CreatePanel<MapViewer>(new MapViewer{ emu });

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}
	

}

void Application::HandleInput()
{
	// INPUT
	if (IsKeyPressed(KEY_UP))
		emu.buttonState.up = true;
	if (IsKeyPressed(KEY_DOWN))
		emu.buttonState.down = true;
	if (IsKeyPressed(KEY_LEFT))
		emu.buttonState.left = true;
	if (IsKeyPressed(KEY_RIGHT))
		emu.buttonState.right = true;
	if (IsKeyPressed(KEY_Z))
		emu.buttonState.a = true;
	if (IsKeyPressed(KEY_X))
		emu.buttonState.b = true;
	if (IsKeyPressed(KEY_ENTER))
		emu.buttonState.start = true;
	if (IsKeyPressed(KEY_LEFT_SHIFT))
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
		emu.buttonState.a = false;
	if (IsKeyReleased(KEY_X))
		emu.buttonState.b = false;
	if (IsKeyReleased(KEY_ENTER))
		emu.buttonState.start = false;
	if (IsKeyReleased(KEY_LEFT_SHIFT))
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
		ImGui::ShowDemoWindow();

		ImGui::DockSpaceOverViewport();


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

		HandleInput();

		if (emu_run && emu.romLoaded) emu.UpdateFrame();
		ImGuiDraw();

		UpdateTexture(renderTexture.texture, emu.ppu.videoBuffer);

		BeginTextureMode(renderTexture);
		//ClearBackground(BLACK);

		//Color defaultColor[4] = { WHITE, LIGHTGRAY, DARKGRAY, BLACK };
		////Color defaultColor[4] = { {155, 188, 15, 255}, {139, 172, 15, 255}, {48, 98, 48, 255}, {15, 56, 15, 255} };
		//for (int y = 0; y < 144; y++)
		//{
		//	for (int x = 0; x < 160; x++)
		//	{
		//		uint32_t colorData = emu.ppu.videoBuffer[y * RESX + x];
		//		DrawPixel(x, y, GetColor(colorData));
		//	}
		//}


		DrawText(std::format("{}", GetFPS()).c_str(), 120, 0, 10, GREEN);

		//if (!m_ErrorMessage.empty())
		//	DrawText(m_ErrorMessage.c_str(), 0, 0, 10, RED);

		//if (!emu_run)
		//{
		//	DrawText("PAUSED", 0, 0, 2, RED);
		//}


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
