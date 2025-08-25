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
	tileMapTexture = LoadRenderTexture(24 * 8, 16 * 8);
	tileIndexTexture = LoadRenderTexture(32 * 8, 32 * 8);

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
				showMemoryView = true;
			if (ImGui::MenuItem("Disassembly"))
				showDisassembly = true;
			if (ImGui::MenuItem("TileMap"))
				showTileMap = true;
			if (ImGui::MenuItem("TileMap Indexes"))
				showTileIndexMap = true;

			ImGui::EndMenu();
		}

		ImGui::EndMainMenuBar();
	}

	if (showMemoryView)
	{
		ImGui::Begin("MemoryView", &showMemoryView);
		ShowMemoryViewText();

		ImGui::End();
	}

	if (showDisassembly)
	{
		ImGui::Begin("Disassembly", &showDisassembly);
		ShowDisassemblyText();
		ImGui::End();
	}
	
	if (showTileMap)
	{
		ImGui::Begin("TileMap", &showTileMap);

		ImGui::Image(tileMapTexture.texture.id, { 24 * 8 * 3, 16 * 8 * 3 }, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

		ImGui::End();
	}

	if (showTileIndexMap)
	{
		ImGui::Begin("TileIndex Map", &showTileIndexMap);

		ShowTileIndexMap();

		ImGui::End();
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
		emu.cpu.RequestInterrupt(INT_JOYPAD);
	}


}

void Application::display_tile(RenderTexture2D texture, uint16_t startLocation, uint16_t tileNum, int x, int y)
{

	Color gameBoyColors[] = {WHITE, LIGHTGRAY, DARKGRAY, BLACK};

	for(int lineY = 0; lineY < 16; lineY += 2)
	{
		uint8_t lo = emu.read(startLocation + (tileNum * 16) + lineY);
		uint8_t hi = emu.read(startLocation + (tileNum * 16) + lineY + 1);

		for(int8_t bit = 7; bit >= 0; bit--)
		{
			uint8_t color = !!(lo & (1 << bit)) | (!!(hi & (1 << bit)) << 1);

			DrawPixel(x + (7 - bit), y + lineY / 2, gameBoyColors[color]);
		}
	}
}

void Application::ShowMemoryViewText()
{
	if (!emu.romLoaded) return;

	ImGui::Text("A: 0x%x", emu.cpu.AF.hi);
	ImGui::SameLine(); ImGui::TextColored(emu.cpu.GetFlag(FLAG_C) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), "C");
	ImGui::SameLine(); ImGui::TextColored(emu.cpu.GetFlag(FLAG_H) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), "H");
	ImGui::SameLine(); ImGui::TextColored(emu.cpu.GetFlag(FLAG_N) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), "N");
	ImGui::SameLine(); ImGui::TextColored(emu.cpu.GetFlag(FLAG_Z) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), "Z");

	ImGui::Text("B: 0x%x", emu.cpu.BC.hi);
	ImGui::Text("C: 0x%x", emu.cpu.BC.lo);
	ImGui::Text("D: 0x%x", emu.cpu.DE.hi);
	ImGui::Text("E: 0x%x", emu.cpu.DE.lo);
	ImGui::Text("HL: 0x%x", emu.cpu.HL.reg);
	ImGui::Text("SP: 0x%x", emu.cpu.SP);
	ImGui::Text("PC: 0x%x", emu.cpu.PC);
	ImGui::Text("Cycles: %d", emu.m_SystemTicks);

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
			for (int i = clipper.DisplayStart * 16; i < clipper.DisplayEnd * 16; i += 16)
			{


				std::string text;
				for (int j = 0; j < 16; j++) {
					if (i + j >= emu.memory.size())
						break;
					text += std::format("{:02X} ", emu.read(i + j)); // Fix formatting
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
}

void Application::ShowDisassemblyText()
{
	if (!emu.romLoaded) return;

	std::vector<std::string> dissassemblylines = disassemble(emu, emu.cpu.PC - 10, emu.cpu.PC + 15);

	for (std::string l : dissassemblylines)
		ImGui::Text(l.c_str());

}

void Application::ShowTileIndexMap()
{
	if (!emu.romLoaded) return;

	ImGui::RadioButton("Tile Map 1", &tileIndexBaseAddress, 0x8000); ImGui::SameLine();
	ImGui::RadioButton("Tile Map 2", &tileIndexBaseAddress, 0x9000);

	ImGui::Image((ImTextureID)tileIndexTexture.texture.id, { 32 * 8 * 3, 32 * 8 * 3 }, ImVec2(0, 1), ImVec2(1, 0));

}

std::vector<std::string> Application::disassemble(Emulator& emu, uint16_t startAddress, uint16_t endAddress)
{
	std::vector<std::string> output;
	for(uint16_t currentAddress = startAddress; currentAddress <= endAddress;)
	{
		uint8_t opcode = emu.read(currentAddress++);
		CPU::Instruction currentInstruction = emu.cpu.InstructionByOpcode(opcode);
		
		std::stringstream ss;

		if(currentAddress == emu.cpu.PC)
			ss << "***";

		ss << std::hex << currentAddress - 1 << " " << std::hex << (int)opcode << " " << currentInstruction.name;
		
		WriteParams(emu, currentInstruction.operand1, ss, currentAddress);
		WriteParams(emu, currentInstruction.operand2, ss, currentAddress);

		output.push_back(ss.str());
	}

	// std::cout << output[0] << std::endl;
	return output;
}


void Application::WriteParams(Emulator& emu, CPU::Operand& op, std::stringstream& ss, uint16_t& currentAddress)
{
			switch(op.mode)
		{
			case CPU::AddressingMode::IMM8:
				ss << " " << std::hex << (int)emu.read(currentAddress++);
				break;
			case CPU::AddressingMode::IMM16:
				ss << " " << std::hex << (int)emu.read16(currentAddress);
				currentAddress += 2;
				break;
			
			case CPU::AddressingMode::REG16:
			case CPU::AddressingMode::REG8:
				ss << " " << RegTypeToString(op.reg);
				break;
			case CPU::AddressingMode::IND:
				ss << " (" << RegTypeToString(op.reg) << ")";
				break;
			case CPU::AddressingMode::IND_IMM8:
				ss << " (" << std::hex << (int)emu.read(currentAddress++) << ")";
				break;
			case CPU::AddressingMode::IND_IMM16:
				ss << " (" << std::hex << (int)emu.read16(currentAddress) << ")";
				currentAddress += 2;
				break;
			case CPU::AddressingMode::COND:
				ss << " " << CondTypeToString(op.cond);

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

		ImGui::Render();
		BeginDrawing();
		ImGui_ImplRaylib_RenderDrawData(ImGui::GetDrawData());
		EndDrawing();


		BeginTextureMode(tileMapTexture);
		ClearBackground(BLUE);
		uint16_t tileNum = 0;


		for (int y = 0; y < 16; y++)
		{
			for (int x = 0; x < 24; x++)
			{
				display_tile(tileMapTexture, 0x8000, tileNum++, x * 8, y * 8);
			}
		}

		EndTextureMode();

		BeginTextureMode(tileIndexTexture);

		for (int y = 0; y < 32; y++)
		{
			for (int x = 0; x < 32; x++)
			{
				int index = y * 32 + x;
				uint16_t address = 0x9800 + index;
				uint8_t tileIndex = emu.read(address);
				display_tile(tileIndexTexture, tileIndexBaseAddress, tileIndex, x * 8, y * 8);
			}
		}

		EndTextureMode();

		BeginTextureMode(renderTexture);
		ClearBackground(BLACK);

		Color defaultColor[4] = { WHITE, LIGHTGRAY, DARKGRAY, BLACK };
		//Color defaultColor[4] = { {155, 188, 15, 255}, {139, 172, 15, 255}, {48, 98, 48, 255}, {15, 56, 15, 255} };
		for (int y = 0; y < 144; y++)
		{
			for (int x = 0; x < 160; x++)
			{
				uint32_t colorData = emu.ppu.videoBuffer[y * RESX + x];
				DrawPixel(x, y, GetColor(colorData));
			}
		}


		DrawText(std::format("{}", GetFPS()).c_str(), 120, 0, 10, GREEN);

		if (!m_ErrorMessage.empty())
			DrawText(m_ErrorMessage.c_str(), 0, 0, 10, RED);

		if (!emu_run)
		{
			DrawText("PAUSED", 0, 0, 2, RED);
		}


 		EndTextureMode();

	}

}
