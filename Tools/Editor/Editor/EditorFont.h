#pragma once

#include "Core/Pattern/Singleton.h"
#include <cassert>

struct ImFont;

class EditorFont : public Singleton<EditorFont>
{
public:
	ImFont* m_fonts[128] = {};

public:
	EditorFont();
	~EditorFont();

private:
	void InitializeFont(int size);

public:
	inline ImFont* GetFont(int size)
	{
		auto& font = m_fonts[size];
		assert(font != nullptr);
		return font;
	}

};

