#include "EditorFont.h"

#include "IconFontCppHeaders/IconsFontAwesome6.h"
#include "imgui/imgui.h"

EditorFont::EditorFont()
{
	// init icon font
	{
		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphOffset.y = 1.0f;
		config.GlyphMinAdvanceX = 23.0f; // Use if you want to make the icon monospaced
		static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.Fonts->AddFontFromFileTTF("Editor/Fonts/FontAwesome6_900.otf", (int)(23.0f), &config, icon_ranges);
	}

	{
		InitializeFont(22);
		InitializeFont(18);
	}
}

EditorFont::~EditorFont()
{
	
}

void EditorFont::InitializeFont(int size)
{
	ImFont*& font = m_fonts[size];

	ImGuiIO& io = ImGui::GetIO(); (void)io;

	ImFontConfig config;
	//config.MergeMode = true;
	config.GlyphOffset.y = 1.0f;
	//config.GlyphMinAdvanceX = float(size);
	config.GlyphRanges = io.Fonts->GetGlyphRangesVietnamese();

	font = io.Fonts->AddFontFromFileTTF("Resources/Default/segoeui.ttf", float(size), &config);

	{
		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphOffset.y = 1.0f;
		config.GlyphMinAdvanceX = float(size); // Use if you want to make the icon monospaced
		static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };

		ImGuiIO& io = ImGui::GetIO(); (void)io;
		io.Fonts->AddFontFromFileTTF("Editor/Fonts/FontAwesome6_900.otf", float(size), &config, icon_ranges);
	}
}
