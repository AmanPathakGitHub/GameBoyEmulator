#pragma once

#include <string>
#include <string_view>
#include <GLFW/glfw3.h>

// TODO SWITCH OUT WITH CROSSPLATFORM LIBRARY IF POSSIBLE

namespace Utils
{
    std::string OpenFileDialog(void* windowHandle, std::string_view filter);
    void ShowMessageBox(void* windowHandle, std::string_view message, std::string_view caption);
}