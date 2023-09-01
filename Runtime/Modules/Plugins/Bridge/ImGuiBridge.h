#pragma once

#include "Core/TypeDef.h"

#ifndef EXPORTS
#define IMGUI_DEFINE_MATH_OPERATORS
#endif

#include "imgui/imgui.h"

#ifndef EXPORTS
#include "imgui/imgui.cpp"
#include "imgui/imgui_draw.cpp"
#include "imgui/imgui_demo.cpp"
#include "imgui/imgui_tables.cpp"
#include "imgui/imgui_widgets.cpp"
#endif // !EXPORTS


NAMESPACE_BEGIN

class API ImGuiBridge
{
public:
	static ImGuiContext* GetContext();
	static void GetAlloctorFunctions(ImGuiMemAllocFunc* output1, ImGuiMemFreeFunc* output2, void** user_data);

	// force it run inside module
	template <typename T = void>
	inline static void InitializeImGui()
	{
		ImGui::SetCurrentContext(GetContext());

		ImGuiMemAllocFunc alloc_func;
		ImGuiMemFreeFunc free_func;
		void* user_data;
		GetAlloctorFunctions(&alloc_func, &free_func, &user_data);

		ImGui::SetAllocatorFunctions(alloc_func, free_func, user_data);
	}

	template <typename T = void>
	inline static void FinalizeImGui()
	{

	}
};



NAMESPACE_END