
#include <windows.h>
#include <commdlg.h>

#include "platformUtils.h"

namespace Utils
{
	std::string OpenFileDialog(void* windowHandle, std::string_view filter)
	{
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
	void ShowMessageBox(void* windowHandle, std::string_view message, std::string_view caption)
	{
		MessageBox((HWND)windowHandle, message.data(), caption.data(), MB_OK);
	}
}