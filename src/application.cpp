#include "application.h"

#include <format>
#include <iostream>
#include <string>

#include <raylib.h>

#include <imgui.h>
#include <rlImGui.h>
#include <imgui_impl_raylib.h>


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

	renderTexture = LoadRenderTexture(160, 144);

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

void Application::DrawRegisters()
{
	DrawText(std::format("A: 0x{:x}", emu.cpu.AF.hi).c_str(), 10, 10, 20, BLACK);
	DrawText(std::format("B: 0x{:x}", emu.cpu.BC.hi).c_str(), 10, 30, 20, BLACK);
	DrawText(std::format("C: 0x{:x}", emu.cpu.BC.lo).c_str(), 10, 50, 20, BLACK);
	DrawText(std::format("D: 0x{:x}", emu.cpu.DE.hi).c_str(), 10, 70, 20, BLACK);
	DrawText(std::format("HL: 0x{:x}", emu.cpu.HL.reg).c_str(), 10, 90, 20, BLACK);
	DrawText(std::format("SP: 0x{:x}", emu.cpu.SP).c_str(), 10, 110, 20, BLACK);
	DrawText(std::format("PC: 0x{:x}", emu.cpu.PC).c_str(), 10, 130, 20, BLACK);
}

void Application::DrawMemory(uint32_t x, uint32_t y, uint16_t startAddress, uint16_t endAddress)
{

	
}

void Application::Run()
{
	while (!WindowShouldClose())    // Detect window close button or ESC key
	{
		ImGui_ImplRaylib_ProcessEvents();

		// Start the Dear ImGui frame
		ImGui_ImplRaylib_NewFrame();
		ImGui::NewFrame();
		bool show = true;
		ImGui::DockSpaceOverViewport();

		ImGui::ShowDemoWindow(&show);

		if (ImGui::BeginMainMenuBar()) {
			if (ImGui::BeginMenu("File")) {
				if (ImGui::MenuItem("Open")) {
					// Handle Open action
				}
				
				ImGui::EndMenu();
			}

			if (ImGui::BeginMenu("Debug")) {
				if (ImGui::MenuItem("Memory View")) {
					// Handle Undo action
				}

				ImGui::EndMenu();
			}

			ImGui::EndMainMenuBar();
		}

		ImGui::Begin("Emulator");
	/*	ImGui::SetCursorPosX(ImGui::GetCursorPosX() + 3);
		ImGui::SetCursorPosY(ImGui::GetCursorPosY() + 3);*/
		ImVec2 windowSize = ImGui::GetContentRegionAvail();

		float scale = std::min(windowSize.x / renderTexture.texture.width, windowSize.y / renderTexture.texture.height);

		ImVec2 scaledSize = ImVec2(renderTexture.texture.width * scale, renderTexture.texture.height * scale);
		
		ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (windowSize.x - scaledSize.x) / 2, ImGui::GetCursorPosY() + (windowSize.y - scaledSize.y) / 2));

		ImGui::Image((ImTextureID)renderTexture.texture.id, scaledSize, ImVec2(0, 1), ImVec2(1, 0));

		ImGui::End();

		ImGui::Begin("MemoryView");
		ImGui::Text("A: 0x%x", emu.cpu.AF.hi);
		ImGui::Text("B: 0x%x", emu.cpu.BC.hi);
		ImGui::Text("C: 0x%x", emu.cpu.BC.lo);
		ImGui::Text("D: 0x%x", emu.cpu.DE.hi);
		ImGui::Text("E: 0x%x", emu.cpu.DE.lo);
		ImGui::Text("HL: 0x%x", emu.cpu.HL.reg);
		ImGui::Text("SP: 0x%x", emu.cpu.SP);
		ImGui::Text("PC: 0x%x", emu.cpu.PC);


		ImGui::PushStyleColor(ImGuiCol_ChildBg, ImGui::GetStyleColorVec4(ImGuiCol_FrameBg));

		static char buf3[4] = ""; ImGui::InputText("Address", buf3, 5, ImGuiInputTextFlags_CharsHexadecimal | ImGuiInputTextFlags_CharsUppercase);

		if (ImGui::BeginChild("ResizableChild", ImVec2(-FLT_MIN, ImGui::GetTextLineHeightWithSpacing() * 8),
			ImGuiChildFlags_Borders | ImGuiChildFlags_ResizeY, ImGuiWindowFlags_HorizontalScrollbar | ImGuiWindowFlags_AlwaysVerticalScrollbar))
		{

			ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));

			ImGuiListClipper clipper;
			clipper.Begin(0x10000 / 16);
			static float cursorY = 0;
			//ImGui::SetScrollFromPosY(ImGui::GetCursorStartPos().y + cursorY, 0.25f);


			int searchNumber = (int)strtol(buf3, NULL, 16);
			searchNumber = searchNumber / 16 * 16; // Align to 16-byte row
			int targetRow = searchNumber / 16;

			// Ensure the row exists
			if (targetRow >= 0 && targetRow < (emu.memory.size() / 16)) {
				// Calculate exact scroll position
				float scrollTarget = targetRow * ImGui::GetTextLineHeightWithSpacing();

				// Set scroll before rendering
				ImGui::SetScrollY(scrollTarget);
			}

			while (clipper.Step())
			{
				for (int i =  clipper.DisplayStart * 16; i < clipper.DisplayEnd * 16; i += 16)
				{


					std::string text;
					for (int j = 0; j < 16; j++) {
						if (i + j >= emu.memory.size())
							break;
						text += std::format("{:02X} ", emu.memory[i + j]); // Fix formatting
					}

					if (i == searchNumber)
					{
						ImGui::TextColored(ImVec4(1, 1, 0, 1), "0x%04X %s", i, text.c_str());
						
					}
					else
						ImGui::Text("0x%04X %s", i, text.c_str());
				}
			}


			ImGui::PopStyleVar();
		}

		ImGui::PopStyleColor();
		ImGui::EndChild();

		ImGui::End();

		
		// Update
		if (IsKeyPressed(KEY_C))
		{
			emu.clock();
		}
		else if (IsKeyPressed(KeyboardKey(KEY_R)))
		{
			emu.cpu.Reset();
		}
		ImGui::Render();
		BeginDrawing();
		ImGui_ImplRaylib_RenderDrawData(ImGui::GetDrawData());
		EndDrawing();

		BeginTextureMode(renderTexture);
		ClearBackground(RAYWHITE);
		DrawText("hello", 0, 0, 5, GREEN);

		EndTextureMode();
	}


}
