#include "mapviewer.h"

#include <imgui.h>

#include <print>

constexpr const char* TITLE = "Map Viewer";
constexpr int SCALE = 3;

MapViewer::MapViewer(Emulator& emu)
	: Panel(TITLE), m_Emulator(emu)
{
	m_RenderTexture = LoadRenderTexture(MAPVIEWER_WIDTH, MAPVIEWER_HEIGHT);
}

MapViewer::MapViewer(const MapViewer& other)
	: Panel(other.m_Name), m_Emulator(other.m_Emulator)
{
	std::println("COPIED");
}

MapViewer::~MapViewer()
{
	UnloadRenderTexture(m_RenderTexture);
}

void MapViewer::Update()
{
	if (!m_Emulator.romLoaded)
	{
		ImGui::Text("Rom not loaded...");
		return;
	}


	ImGui::RadioButton("Tile Map 1", &startAddress, 0x9800); ImGui::SameLine();
	ImGui::RadioButton("Tile Map 2", &startAddress, 0x9C00);

	UpdatePixelBuffer(startAddress);
	UpdateTexture(m_RenderTexture.texture, m_PixelBuffer.data());


	ImVec2 windowSize = ImGui::GetContentRegionAvail();

	float scale = std::min(windowSize.x / m_RenderTexture.texture.width, windowSize.y / m_RenderTexture.texture.height);

	ImVec2 scaledSize = ImVec2(m_RenderTexture.texture.width * scale, m_RenderTexture.texture.height * scale);

	ImGui::SetCursorPos(ImVec2(ImGui::GetCursorPosX() + (windowSize.x - scaledSize.x) / 2, ImGui::GetCursorPosY() + (windowSize.y - scaledSize.y) / 2));

	ImGui::Image(m_RenderTexture.texture.id, scaledSize);
}

void MapViewer::UpdatePixelBuffer(uint16_t startLocation)
{

	for (int y = 0; y < 32; y++)
	{
		for (int x = 0; x < 32; x++)
		{
			uint16_t index = y * 32 + x;
			uint16_t address = startLocation + index;
			uint8_t tileIndex = m_Emulator.read(address);

			for (int lineY = 0; lineY < 16; lineY += 2)
			{
				uint16_t tileAddress = m_Emulator.lcd.GetControlBit(LCD::Control::BG_WINDOW_TILES) ? 0x8000 + tileIndex * 16 : 0x9000 + (int8_t)tileIndex * 16;

				uint8_t lo = m_Emulator.read(tileAddress + lineY);
				uint8_t hi = m_Emulator.read(tileAddress + lineY + 1);

				for (int8_t bit = 7; bit >= 0; bit--)
				{
					uint8_t color = !!(lo & (1 << bit)) | (!!(hi & (1 << bit)) << 1);

					m_PixelBuffer[((y * 8 + lineY / 2) * MAPVIEWER_WIDTH) + (x * 8 + (7 - bit))] = DEFAULT_COLORS[color];

				}
			}
		}
	}
}
