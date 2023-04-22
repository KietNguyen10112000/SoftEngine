#include "ImGuiBridge.h"

NAMESPACE_BEGIN

ImGuiContext* ImGuiBridge::GetContext()
{
	return ImGui::GetCurrentContext();
}

void ImGuiBridge::GetAlloctorFunctions(ImGuiMemAllocFunc* output1, ImGuiMemFreeFunc* output2, void** user_data)
{
	ImGui::GetAllocatorFunctions(output1, output2, user_data);
}

NAMESPACE_END