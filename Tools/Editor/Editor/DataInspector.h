#pragma once
#include "Common/Base/Metadata.h"

using namespace soft;

class DataInspector
{
public:
	constexpr static size_t MAX_TYPE = 256;

	using InspectFunc = bool (*)(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);

	static InspectFunc s_inspectFunc[MAX_TYPE];

	// variant is the value from calling accessor.Get() so don't need to re-call accessor.Get() inside InspectFunc

	static bool InspectBool(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);
	static bool InspectFloat(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);
	static bool InspectUint64(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);
	static bool InspectVec3(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);

	static bool InspectTransformEx(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName, 
		bool hideScale = false, Vec3* outputRotateAxis = nullptr);
	static bool InspectTransform(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);

	static bool InspectProjectionMat4(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);

	static bool InspectString(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);
	static bool InspectStringPathEx(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName,
		bool allowOutsideResources, float width = 0, bool directory = false, const String& startPath = {}, bool directOpenSystemDialog = false);
	static bool InspectStringPath(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);


public:
	static void Initialize();
	static bool Inspect(ClassMetadata* metadata, Accessor& accessor, const char* propertyName);

};

