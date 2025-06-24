#include "application.h"

#include <format>
#include <iostream>
#include <string>
#include <sstream>

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
	tileMapTexture = LoadRenderTexture(24 * 8, 16 * 8);


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

	emu.LoadROM("Dr. Mario (World).gb");

}

Application::~Application()
{

	ImGui_ImplRaylib_Shutdown();
	ImGui::DestroyContext();

	CloseWindow();

}

static int scale = 5;



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

void Application::DrawTiles()
{


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

std::vector<std::string> Application::dissassemble(Emulator& emu, uint16_t startAddress, uint16_t endAddress)
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

std::string Application::RegTypeToString(CPU::RegType reg)
{
	switch (reg)
	{
		case CPU::RegType::NONE: return "NONE";
        case CPU::RegType::A:    return "A";
        case CPU::RegType::B:    return "B";
        case CPU::RegType::C:    return "C";
        case CPU::RegType::D:    return "D";
        case CPU::RegType::E:    return "E";
        case CPU::RegType::H:    return "H";
        case CPU::RegType::L:    return "L";
        case CPU::RegType::AF:   return "AF";
        case CPU::RegType::BC:   return "BC";
        case CPU::RegType::DE:   return "DE";
        case CPU::RegType::HL:   return "HL";
        case CPU::RegType::SP:   return "SP";
        case CPU::RegType::HLI:  return "HLI";
        case CPU::RegType::HLD:  return "HLD";
        default: return "UNKNOWN";
	}
}

std::string Application::CondTypeToString(CPU::CondType cond)
{
	switch (cond)
    {
        case CPU::CondType::NONE: return "NONE";
        case CPU::CondType::Z:    return "Z";
        case CPU::CondType::NZ:   return "NZ";
        case CPU::CondType::C:    return "C";
        case CPU::CondType::NC:   return "NC";
        default: return "UNKNOWN";
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
				for (int i =  clipper.DisplayStart * 16; i < clipper.DisplayEnd * 16; i += 16)
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

		ImGui::End();

		ImGui::Begin("Disassembly");

		std::vector<std::string> dissassemblylines = dissassemble(emu, emu.cpu.PC, emu.cpu.PC + 15);

		for(std::string l : dissassemblylines)
			ImGui::Text(l.c_str());

		ImGui::End();

		
		// Update

		//tileMapTexture = LoadRenderTexture(24 * 8, 16 * 8);



		ImGui::Begin("TileMap");

		ImGui::Image(tileMapTexture.texture.id, {24 * 8 * scale, 16 * 8 * scale}, ImVec2(0.0f, 1.0f), ImVec2(1.0f, 0.0f));

		ImGui::End();



		// if(msg[0])
		// {
		// 	std::cout << msg << std::endl;
		// }

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
			emu_run  = !emu_run;
		}
		else if (IsKeyPressed(KEY_P))
		{
			for(int i = 0; i < 100; i++)
				emu.clock(); 
		}
		else if (IsKeyPressed(KEY_K))
		{
			for(int i = 0; i < 20; i++)
				emu.clock(); 
		}



		if(emu_run) emu.UpdateFrame();



		DrawTiles();

		ImGui::Render();
		BeginDrawing();
		ImGui_ImplRaylib_RenderDrawData(ImGui::GetDrawData());
		EndDrawing();

		BeginTextureMode(tileMapTexture);
		ClearBackground(BLUE);
		uint16_t tileNum = 0;

	

		for(int y = 0; y < 16; y++)
		{
			for(int x = 0; x < 24; x++)
			{
				display_tile(tileMapTexture, 0x8000, tileNum++, x * 8, y * 8);
			}
		}


		
		EndTextureMode();

		BeginTextureMode(renderTexture);
		ClearBackground(RAYWHITE);
		DrawText("hello", 0, 0, 5, GREEN);

		EndTextureMode();





	}

}
