#pragma once

// This file is to seperate platorm specific code (so far only the dialog box)
// also raylib and the win32 api (windows.h and raylib.h) have naming conflicts so they can't be included in the same file


#define GLFW_EXPOSE_NATIVE_WIN32

#include <windows.h>
#include <commdlg.h>
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include "platformUtils.h"

namespace Utils
{
	// thx the cherno
	std::string OpenFileDialog(void* windowHandle, std::string_view filter)
	{
		// WINDOWS ONLY
		OPENFILENAME fileData;

		char fileBuffer[300] = { 0 };

		memset((char*)&fileData, 0, sizeof(OPENFILENAME));
		fileData.lStructSize = sizeof(OPENFILENAME);

		fileData.hwndOwner = (HWND)windowHandle;

		fileData.lpstrFilter = filter.data();
		fileData.nFilterIndex = 1;
		fileData.Flags = OFN_FILEMUSTEXIST | OFN_PATHMUSTEXIST;
		fileData.lpstrFile = fileBuffer;
		fileData.nMaxFile = sizeof(fileBuffer);

		if (GetOpenFileName(&fileData))
			return fileData.lpstrFile;


		return "";

	}
}