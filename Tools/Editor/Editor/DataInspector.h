#pragma once
#include "Common/Base/Metadata.h"

using namespace soft;

class DataInspector
{
private:
	constexpr static size_t MAX_TYPE = 256;

	using InspectFunc = void (*)(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);

	static InspectFunc s_inspectFunc[MAX_TYPE];

	// variant is the value from calling accessor.Get() so don't need to re-call accessor.Get() inside InspectFunc
	static void InspectFloat(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);
	static void InspectVec3(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);
	static void InspectTransform(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);
	static void InspectProjectionMat4(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);

	static void InspectStringPath(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName);

public:
	static void Initialize();
	static void Inspect(ClassMetadata* metadata, Accessor& accessor, const char* propertyName);

};

