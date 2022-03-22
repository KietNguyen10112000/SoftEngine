#include "ImGuiCommon.h"

#include "Engine/Scripting/ScriptEngine.h"
#include "Engine/Scripting/ScriptLanguage.h"
#include "Engine/Scene/Scene.h"

#include "Core/Common.h"

void InitImgui(HWND hwnd)
{
	//auto hwnd = window->GetNativeHandle();
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void)io;
	ImFontConfig config;
	config.SizePixels = 15;
	config.OversampleH = config.OversampleV = 1;
	config.PixelSnapH = true;
	io.Fonts->AddFontDefault(&config);
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	//io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsClassic();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hwnd);
	ImGui_ImplDX11_Init(DX11Global::renderer->m_d3dDevice, DX11Global::renderer->m_d3dDeviceContext);

	constexpr int GWL_WNDPROC_ = -4;
	g_oldHWNDHandle = (WNDPROC)SetWindowLongPtr(hwnd, GWL_WNDPROC_, (LONG_PTR)WndHandle2);
}

void ImGuiCommon::Console::Update(Scene* scene)
{
	static std::string prevBuffer;
	static std::string buffer(128, '\0');
	static std::vector<std::string> list;
	static std::vector<const char*> listView;
	static const char* currentItem = 0;

	ImGui::Begin("Console");

	auto scriptEngine = scene->GetScriptEngine();

	if (scriptEngine)
	{
		auto lang = scriptEngine->GetLanguage();
		
		std::fill(buffer.begin(), buffer.end(), '\0');
		lang->GetName(&buffer[0], buffer.size());

		ImGui::Text("Script language: ");
		ImGui::SameLine();
		ImGui::TextColored({ 0,1,0,1 }, buffer.c_str());

		std::fill(buffer.begin(), buffer.end(), '\0');
		lang->EnumVersions(&buffer[0], buffer.size());

		if (prevBuffer != buffer)
		{
			list.clear();
			listView.clear();
			StringSplit(buffer, ";", list);

			for (auto& item : list)
			{
				listView.push_back(item.c_str());
			}

			prevBuffer = buffer;
		}

		if (ImGui::BeginCombo("Version", currentItem))
		{
			for (size_t n = 0; n < list.size(); n++)
			{
				bool is_selected = (currentItem == listView[n]);
				if (ImGui::Selectable(listView[n], is_selected))
					currentItem = listView[n];
				if (is_selected)
					ImGui::SetItemDefaultFocus();
			}
			ImGui::EndCombo();
		}

	}

	if (ImGui::Button("Run"))
	{
		m_resizedInput = &m_input[0];

		RedirectStdOutput();

		if (scriptEngine)
		{
			scriptEngine->Run(m_resizedInput.c_str());
		}
		else
		{
			std::cerr << "Cannot get script engine to run script\n";
		}

		RestoreStdOutput();

	}

	ImGui::SameLine();

	if (ImGui::Button("Clear"))
	{
		ClearOutput();
	}

	ImGui::Separator();

	const float height = ImGui::GetWindowHeight() / 2.0f;
	ImGui::BeginChild("ScrollingRegion", ImVec2(0, height), false, ImGuiWindowFlags_HorizontalScrollbar);

	ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(4, 1)); // Tighten spacing
	for (auto& log : m_logHistoryView)
	{
		auto error = log.second;

		auto* list = (!error) ? &m_outHistory : &m_errHistory;

		if (error)
		{
			ImGui::PushStyleColor(ImGuiCol_Text, ImVec4(1.0f, 0.4f, 0.4f, 1.0f));
		}
		ImGui::TextUnformatted((*list)[log.first].c_str());

		if (error)
		{
			ImGui::PopStyleColor();
		}
	}
	ImGui::PopStyleVar();

	ImGui::EndChild();
	ImGui::Separator();

	ImGui::InputTextMultiline("##source", &m_input[0], INPUT_BUF_SIZE,
		ImVec2(-FLT_MIN, height - (scriptEngine ? 120 : 80)), ImGuiInputTextFlags_AllowTabInput);

	ImGui::End();
}
