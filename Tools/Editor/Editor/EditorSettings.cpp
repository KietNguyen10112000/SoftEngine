#include "EditorSettings.h"

#include "imgui/imgui.h"

void EditorSettings::Render()
{
	if (ImGui::TreeNode("General Setting"))
	{
		ImGui::DragFloat("Scaling Adjustment Precise", &GeneralSetting.scalingAdjustmentPrecision,
			0.0000001f, 0.0000001f, 0.1f, "%f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		ImGui::DragFloat("Rotation Adjustment Precise", &GeneralSetting.rotationAdjustmentPrecision,
			0.0000001f, 0.0000001f, 0.1f, "%f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);
		ImGui::DragFloat("Position Adjustment Precise", &GeneralSetting.positionAdjustmentPrecision,
			0.0000001f, 0.0000001f, 0.1f, "%f", ImGuiSliderFlags_::ImGuiSliderFlags_AlwaysClamp);

		ImGui::TreePop();
	}
}

void EditorSettings::OnApplySetting()
{
}

const char* EditorSettings::GetPrecisionCFormatStr(float precision)
{
	// =))), simply and effectively work
	if (precision < 0.000001f)
	{
		return "%f";
	}

	if (precision < 0.00001f)
	{
		return "%.5f";
	}

	if (precision < 0.0001f)
	{
		return "%.4f";
	}

	if (precision < 0.001f)
	{
		return "%.3f";
	}

	if (precision < 0.01f)
	{
		return "%.2f";
	}

	if (precision < 0.1f)
	{
		return "%.1f";
	}

	return nullptr;
}
