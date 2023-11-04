#include "DataInspector.h"

#include "imgui/imgui.h"
#include "ImGuiExtern.h"

#include "Runtime/Runtime.h"
#include "Input/Input.h"

DataInspector::InspectFunc DataInspector::s_inspectFunc[MAX_TYPE] = {};

void DataInspector::InspectTransform(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName)
{
	const static char* cacheNameFmt = "editor_InspectTransform_{}";
	struct TransformCache
	{
		Transform transform;
		Vec3 euler;
	};

	Transform transform = variant.As<Transform>();

	Vec3 euler;

	auto cacheName = String::Format(cacheNameFmt, propertyName);

	auto cache = metadata->GenericDictionary()->Get<TransformCache>(cacheName);
	if (!cache || !cache->transform.Equals(transform))
	{
		euler = transform.Rotation().ToEulerAngles();

		if (!cache)
		{
			cache = mheap::New<TransformCache>();
			metadata->GenericDictionary()->Store(cacheName, cache);
		}

		cache->transform = transform;
		cache->euler = euler;
	}
	else
	{
		euler = cache->euler;
	}

	bool modified = false;
	modified |= ImGui::DragFloat3("Scale", &transform.Scale()[0], 0.01f, -INFINITY, INFINITY);
	modified |= ImGui::DragFloat3("Rotation", &euler[0], 0.01f, -INFINITY, INFINITY);
	modified |= ImGui::DragFloat3("Position", &transform.Position()[0], 0.01f, -INFINITY, INFINITY);

	if (modified)
	{
		transform.Rotation() = Quaternion(euler);

		auto input = Variant::Of<Transform>();
		input.As<Transform>() = transform;
		accessor.Set(input);

		cache->transform = transform;
		cache->euler = euler;

		//std::cout << cache->euler.x << ", " << cache->euler.y << ", " << cache->euler.z << "\n";
	}
}

void DataInspector::InspectProjectionMat4(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName)
{
	const static char* cacheNameFmt = "editor_InspectProjectionMat4_{}";
	struct ProjectionMat4Cache
	{
		struct PerspectiveCache
		{
			float fovY;
			float aspect;
			float near;
			float far;
		};

		struct OrthographicCache
		{
			float width;
			float height;
			float near;
			float far;
		};

		Mat4 projectionMat;

		union
		{
			PerspectiveCache perspective;
			OrthographicCache orthographic;
		};
		
		uint32_t isPerspective;
	};

	auto& projectionMat = variant.As<Mat4>();
	float fovY = 0;
	float aspect = 0;
	float near = 0;
	float far = 0;
	float width = 0;
	float height = 0;

	auto cacheName = String::Format(cacheNameFmt, propertyName);

	auto cache = metadata->GenericDictionary()->Get<ProjectionMat4Cache>(cacheName);
	if (!cache || std::memcmp(&cache->projectionMat, &projectionMat, sizeof(Mat4)) != 0)
	{
		if (!cache)
		{
			cache = mheap::New<ProjectionMat4Cache>();
			metadata->GenericDictionary()->Store(cacheName, cache);
		}

		cache->projectionMat = projectionMat;

		if (projectionMat[3][3])
		{
			// ortho
			cache->isPerspective = false;

			width = 2.0f / projectionMat[0][0];
			height = 2.0f / projectionMat[1][1];
			near = -projectionMat[3][2] / projectionMat[2][2];
			far = (1.0f / projectionMat[2][2]) + near;

			cache->orthographic.width = width;
			cache->orthographic.height = height;
			cache->orthographic.near = near;
			cache->orthographic.far = far;
		}
		else
		{
			// perspective
			cache->isPerspective = true;

			fovY = 2.0f * std::atan(1 / projectionMat[1][1]);
			aspect = projectionMat[1][1] / projectionMat[0][0];
			near = -projectionMat[3][2] / projectionMat[2][2];
			far = near / (1.0f - 1.0f / projectionMat[2][2]);

			cache->perspective.fovY = fovY;
			cache->perspective.aspect = aspect;
			cache->perspective.near = near;
			cache->perspective.far = far;
		}

	}
	else
	{
		if (!cache->isPerspective)
		{
			width = cache->orthographic.width;
			height = cache->orthographic.height;
			near = cache->orthographic.near;
			far = cache->orthographic.far;
		}
		else
		{
			fovY = cache->perspective.fovY;
			aspect = cache->perspective.aspect;
			near = cache->perspective.near;
			far = cache->perspective.far;
		}
	}

	bool modified = false;

	if (cache->isPerspective)
	{
		modified |= ImGui::DragFloat("FOV Y", &fovY, 0.01f, 0.05f, INFINITY);
		modified |= ImGui::DragFloat("Aspect ratio", &aspect, 0.01f, 0.05f, INFINITY);
		modified |= ImGui::DragFloat("Near", &near, 0.01f, 0.05f, INFINITY);
		modified |= ImGui::DragFloat("Far", &far, 0.01f, 0.05f, INFINITY);
	}
	else
	{
		modified |= ImGui::DragFloat("Width", &width, 0.01f, 0.05f, INFINITY);
		modified |= ImGui::DragFloat("Height", &height, 0.01f, 0.05f, INFINITY);
		modified |= ImGui::DragFloat("Near", &near, 0.01f, -INFINITY, INFINITY);
		modified |= ImGui::DragFloat("Far", &far, 0.01f, 0.05f, INFINITY);
	}
	
	bool changeType = false;
	bool isPerspective = cache->isPerspective;
	ImGui::Text(isPerspective ? "Perspective" : "Orthographic");
	ImGui::SameLine();
	ImGui::ToggleButton("Perspective", &isPerspective);
	if (isPerspective != (bool)cache->isPerspective)
	{
		changeType = true;
	}

	if (changeType)
	{
		auto input = Runtime::Get()->GetInput();

		modified = true;
		cache->isPerspective = !((bool)cache->isPerspective);
		if (cache->isPerspective)
		{
			fovY = PI / 3;
			aspect = input->GetClientWidth() / (float)input->GetClientHeight();
			near = 0.5f;
			far = 1000.0f;
		}
		else
		{
			width = input->GetClientWidth();
			height = input->GetClientHeight();
			near = 0.5f;
			far = 1000.0f;
		}
	}

	if (modified)
	{
		Mat4 mat;
		if (cache->isPerspective)
		{
			mat.SetPerspectiveFovLH(fovY, aspect, near, far);

			cache->perspective.fovY = fovY;
			cache->perspective.aspect = aspect;
			cache->perspective.near = near;
			cache->perspective.far = far;
		}
		else
		{
			mat.SetOrthographicLH(width, height, near, far);

			cache->orthographic.width = width;
			cache->orthographic.height = height;
			cache->orthographic.near = near;
			cache->orthographic.far = far;
		}

		cache->projectionMat = mat;

		auto input = Variant::Of<Mat4>();
		input.As<Mat4>() = mat;
		accessor.Set(input);
	}
}

void DataInspector::Inspect(ClassMetadata* metadata, Accessor& accessor, const char* propertyName)
{
	auto variant = accessor.Get();
	auto func = s_inspectFunc[variant.Type()];
	if (func)
	{
		func(metadata, accessor, variant, propertyName);
	}
}

void DataInspector::Initialize()
{
	s_inspectFunc[VARIANT_TYPE::TRANSFORM3D]				= InspectTransform;
	s_inspectFunc[VARIANT_TYPE::PROJECTION_MAT4]			= InspectProjectionMat4;
}