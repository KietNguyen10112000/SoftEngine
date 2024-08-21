#pragma once

#include "Core/Pattern/Singleton.h"

#include "Math/Math.h"

class EditorSettings : public Singleton<EditorSettings>
{
public:
	struct
	{
		float scalingAdjustmentPrecision = 0.001f;
		float rotationAdjustmentPrecision = 0.0001f;
		float positionAdjustmentPrecision = 0.001f;
	} GeneralSetting;

	void Render();
	void OnApplySetting();

	const char* GetPrecisionCFormatStr(float precision);

};