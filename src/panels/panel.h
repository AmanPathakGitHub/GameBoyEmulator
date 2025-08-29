#pragma once

#include <string>

// Wrapper for an ImGui Window
class Panel
{
public:

	Panel(const std::string& name)
		: m_Name(name)
	{
	}

	Panel(std::string&& name)
		: m_Name(std::move(name))
	{
	}

	virtual void Update() = 0; // ImGui drawing should be done here aswell

public:
	bool m_IsVisible = true;
	std::string m_Name;
};