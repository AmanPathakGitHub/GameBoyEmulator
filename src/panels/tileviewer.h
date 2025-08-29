#pragma once

#include "panel.h"
#include "emulator.h"

#include <raylib.h>

constexpr uint32_t TILEVIEWER_WIDTH = 24 * 8;
constexpr uint32_t TILEVIEWER_HEIGHT = 16 * 8;

class TileViewer : public Panel
{
public:
	TileViewer(Emulator& emu);
	~TileViewer();

	void Update() override;

private:
	Emulator& m_Emulator;
	std::array<uint32_t, TILEVIEWER_WIDTH * TILEVIEWER_HEIGHT> m_PixelBuffer;
	
	RenderTexture2D m_Texture;

	void UpdatePixelBuffer();
};