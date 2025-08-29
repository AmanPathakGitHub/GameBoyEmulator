#include "memoryview.h"

#include <imgui.h>
#include <print>

MemoryView::MemoryView(Emulator& emu, const std::string& name)
	:	Panel(name), m_Emulator(emu)
{
}


MemoryView::MemoryView(Emulator& emu, std::string&& name)
	: Panel(std::move(name)), m_Emulator(emu)
{
}

void MemoryView::Update()
{
	Emulator& emu = m_Emulator;
	if (!emu.romLoaded) return;



	ImGui::Text("A: 0x%x", emu.cpu.AF.hi);
	ImGui::SameLine(); ImGui::TextColored(emu.cpu.GetFlag(CPU::C) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), "C");
	ImGui::SameLine(); ImGui::TextColored(emu.cpu.GetFlag(CPU::H) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), "H");
	ImGui::SameLine(); ImGui::TextColored(emu.cpu.GetFlag(CPU::N) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), "N");
	ImGui::SameLine(); ImGui::TextColored(emu.cpu.GetFlag(CPU::Z) ? ImVec4(0, 1, 0, 1) : ImVec4(1, 0, 0, 1), "Z");

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