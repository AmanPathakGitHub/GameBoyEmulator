#pragma once

#include "panel.h"
#include "emulator.h"
#include <raylib.h>

constexpr uint32_t MAPVIEWER_WIDTH = 32 * 8;
constexpr uint32_t MAPVIEWER_HEIGHT = 32 * 8;

class MapViewer : public Panel
{
public:
	MapViewer(Emulator& emu);
	MapViewer(const MapViewer& other);
	~MapViewer();

	void Update() override;

private:
	Emulator& m_Emulator;
	
	RenderTexture2D m_RenderTexture;
	std::array<uint32_t, MAPVIEWER_WIDTH * MAPVIEWER_HEIGHT> m_PixelBuffer;

	int startAddress = 0x9C00;

	void UpdatePixelBuffer(uint16_t startLocation);
};