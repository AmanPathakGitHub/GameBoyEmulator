#include "tileviewer.h"

#include <imgui.h>

static constexpr const char* TITLE = "Tile Viewer";
static constexpr int SCALE = 3;

TileViewer::TileViewer(Emulator& emu)
	: Panel(TITLE), m_Emulator(emu), m_PixelBuffer()
{
	m_Texture = LoadRenderTexture(TILEVIEWER_WIDTH, TILEVIEWER_HEIGHT);
	
	std::fill(m_PixelBuffer.begin(), m_PixelBuffer.end(), 0xFFFFFFFF);
}

TileViewer::~TileViewer()
{
	UnloadRenderTexture(m_Texture);
}

void TileViewer::Update()
{
	UpdatePixelBuffer();
	UpdateTexture(m_Texture.texture, m_PixelBuffer.data());

	ImGui::Image(m_Texture.texture.id, { TILEVIEWER_WIDTH * SCALE, TILEVIEWER_HEIGHT * SCALE });

}

void TileViewer::UpdatePixelBuffer()
{
	constexpr uint16_t startLocation = 0x8000;

	int tileNum = 0;	

	for (int y = 0; y < TILEVIEWER_HEIGHT; y += 8)
	{
		for (int x = 0; x < TILEVIEWER_WIDTH; x += 8)
		{
			for (int lineY = 0; lineY < 16; lineY += 2)
			{
				uint8_t lo = m_Emulator.read(startLocation + (tileNum * 16) + lineY);
				uint8_t hi = m_Emulator.read(startLocation + (tileNum * 16) + lineY + 1);

				for (int8_t bit = 7; bit >= 0; bit--)
				{
					uint8_t color = !!(lo & (1 << bit)) | (!!(hi & (1 << bit)) << 1);

					m_PixelBuffer[((y + lineY / 2) * TILEVIEWER_WIDTH) + (x + (7 - bit))] = DEFAULT_COLORS[color];

				}
			}
			tileNum++;
		}
	}

	printf("%d\n", tileNum);

}