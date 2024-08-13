#include "DataInspector.h"

#include "imgui/imgui.h"
#include "ImGuiExtern.h"

#include "Runtime/Runtime.h"
#include "Input/Input.h"

#include "FileSystem/FileSystem.h"
#include "FileSystem/FileUtils.h"

#include "IconFontCppHeaders/IconsFontAwesome6.h"

#ifdef _WIN32
#include <Windows.h>
#include <shlobj_core.h>
#undef near
#undef far
#endif

#include "Graphics/Graphics.h"
#include "Graphics/DebugGraphics.h"

#ifdef min
#undef min
#undef max
#endif // min

#include "EditorContext.h"

DataInspector::InspectFunc DataInspector::s_inspectFunc[MAX_TYPE] = {};

bool DataInspector::InspectTransformEx(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName, 
	bool hideScale, Vec3* outputRotateAxis)
{
	struct TransformCopyData
	{
		Transform copiedTransform;
	};

	const static char* cacheNameFmt = "editor_InspectTransform_{}";
	struct TransformCache
	{
		Transform transform;
		Vec3 euler;

		// 0 - Euler
		// 1 - around Right
		// 2 - around Up
		// 3 - around Forward
		byte rotationInspectType = 0;
		float rotationOffset = 0;
		Vec3 rotationAxis;
		ImVec4 rotationAxisColor;
		Quaternion startQuat;
	};

	Transform transform = variant.As<Transform>();

	Vec3 euler;

	auto cacheName = String::Format(cacheNameFmt, propertyName);

	auto cache = metadata->GenericDictionary()->Get<TransformCache>(cacheName);
	if (!cache || !cache->transform.Equals(transform, 0.01f))
	{
		euler = transform.Rotation().ToEulerAngles();

		if (!cache)
		{
			cache = mheap::New<TransformCache>();
			metadata->GenericDictionary()->Store(cacheName, cache);
		}

		cache->transform = transform;
		cache->euler = euler;
		cache->startQuat = transform.Rotation();
	}
	else
	{
		euler = cache->euler;
	}

	bool modified = false;

	if (!hideScale)
	{
		modified |= ImGui::DragFloatN_Colored("Scale", &transform.Scale()[0], 3, 0.001f, -INFINITY, INFINITY);
	}

	ImVec2 cursorPos = { 0,0 };
	if (cache->rotationInspectType == 0)
	{
		modified |= ImGui::DragFloatN_Colored("Rotation    ", &euler[0], 3, 0.001f, -INFINITY, INFINITY);
	}
	else
	{
		modified |= ImGui::DragFloat("## Rotation", &cache->rotationOffset, 0.001f, -INFINITY, INFINITY);

		ImGuiWindow* window = ImGui::GetCurrentWindow();
		ImGuiContext& g = *GImGui;
		const ImVec2 min = ImGui::GetItemRectMin();
		const ImVec2 max = ImGui::GetItemRectMax();
		const float spacing = g.Style.FrameRounding;
		const float halfSpacing = spacing / 2;

		const ImU32 s_colors[] = {
			0xBB0000FF, // red
			0xBB00FF00, // green
			0xBBFF0000, // blue
			0xBBFFFFFF, // white for alpha?
		};

		window->DrawList->AddLine({ min.x + spacing, max.y - halfSpacing }, { max.x - spacing, max.y - halfSpacing }, s_colors[cache->rotationInspectType - 1], 4);

		ImGui::SameLine(); 

		cursorPos = ImGui::GetCursorPos();
		
		//ImGui::PushStyleColor(ImGuiCol_::ImGuiCol_Text, cache->rotationAxisColor);
		String text;
		switch (cache->rotationInspectType)
		{
		case 1:
			text = "Rotation X";
			break;
		case 2:
			text = "Rotation Y";
			break;
		case 3:
			text = "Rotation Z";
			break;
		default:
			break;
		}

		auto pos = ImGui::GetCursorPos();
		ImGui::SetCursorPos(ImVec2(pos.x - 4, pos.y));
		ImGui::TextUnformatted(text.c_str());
		//ImGui::PopStyleColor();
	}

	ImGui::SameLine();
	float btnPosX = 0;
	if (cursorPos.x != 0)
	{
		btnPosX = cursorPos.x + ImGui::CalcTextSize("Rotation    ").x + 4;
		ImGui::SetCursorPos(ImVec2(btnPosX, cursorPos.y));
	}

	btnPosX = ImGui::GetCursorPos().x;
	if (ImGui::Button(ICON_FA_ROTATE "## switch rotation btn"))
	{
		cache->rotationInspectType = (cache->rotationInspectType + 1) % 4;
		cache->rotationOffset = 0;
		cache->startQuat = transform.Rotation();
		switch (cache->rotationInspectType)
		{
		case 0:
			cache->euler = transform.Rotation().ToEulerAngles();
			cache->rotationAxis = Vec3::ZERO;
			break;
		case 1:
			cache->rotationAxis = transform.ToTransformMatrix().Right().Normal();
			cache->rotationAxisColor = { 1,0,0,1 };
			break;
		case 2:
			cache->rotationAxis = transform.ToTransformMatrix().Up().Normal();
			cache->rotationAxisColor = { 0,1,0,1 };
			break;
		case 3:
			cache->rotationAxis = transform.ToTransformMatrix().Forward().Normal();
			cache->rotationAxisColor = { 0,0,1,1 };
			break;
		default:
			break;
		}
	}

	//Graphics::Get()->GetDebugGraphics()->DrawDirection(transform.GetPosition(), cache->rotationAxis * 20.0f);

	modified |= ImGui::DragFloatN_Colored("Position", &transform.Position()[0], 3, 0.001f, -INFINITY, INFINITY);

	ImGui::SameLine();
	cursorPos = ImGui::GetCursorPos();
	ImGui::SetCursorPos(ImVec2(btnPosX - 20, cursorPos.y));
	if (ImGui::Button(ICON_FA_COPY "## copy transform btn"))
	{
		auto data = EditorContext::Get()->GenericDictionary()->Get<TransformCopyData>("TransformCopyData");
		if (!data)
		{
			data = mheap::New<TransformCopyData>();
			EditorContext::Get()->GenericDictionary()->Store("TransformCopyData", data);
		}

		data->copiedTransform = cache->transform;
	}

	ImGui::SameLine();
	if (ImGui::Button(ICON_FA_PASTE "## paste transform btn"))
	{
		auto data = EditorContext::Get()->GenericDictionary()->Get<TransformCopyData>("TransformCopyData");
		if (data)
		{
			modified = true;
			transform = data->copiedTransform;
		}
	}

	if (modified)
	{
		switch (cache->rotationInspectType)
		{
		case 1:
		case 2:
		case 3:
			transform.Rotation() = Mat4::Rotation(cache->startQuat) * Mat4::Rotation(cache->rotationAxis, cache->rotationOffset);
			break;
		default:
			transform.Rotation() = Quaternion(euler);
			break;
		}

		auto input = Variant::Of<Transform>();
		input.As<Transform>() = transform;
		accessor.Set(input);

		cache->transform = transform;
		cache->euler = euler;

		//std::cout << cache->euler.x << ", " << cache->euler.y << ", " << cache->euler.z << "\n";
	}

	if (outputRotateAxis)
	{
		*outputRotateAxis = cache->rotationAxis;
	}
	return modified;
}

bool DataInspector::InspectTransform(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName)
{
	return InspectTransformEx(metadata, accessor, variant, propertyName);
}

bool DataInspector::InspectBool(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName)
{
	auto& v = variant.As<bool>();
	auto name = "## " + String(propertyName);
	if (ImGui::Checkbox(name.c_str(), &v))
	{
		auto input = Variant::Of<bool>();
		input.As<bool>() = v;
		accessor.Set(input);

		return true;
	}

	return false;
}

bool DataInspector::InspectFloat(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName)
{
	auto& v = variant.As<float>();
	auto name = "## " + String(propertyName);
	if (ImGui::DragFloat(name.c_str(), &v, 0.01f, -INFINITY, INFINITY))
	{
		auto input = Variant::Of<float>();
		input.As<float>() = v;
		accessor.Set(input);

		return true;
	}

	return false;
}

bool DataInspector::InspectUint64(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName)
{
	auto& v = variant.As<uint64_t>();
	int temp = (int)v;
	auto name = "## " + String(propertyName);
	if (ImGui::DragInt(name.c_str(), (int*)&temp, 0.1f, -INT_MAX, INT_MAX))
	{
		auto input = Variant::Of<uint64_t>();
		input.As<uint64_t>() = temp;
		accessor.Set(input);

		return true;
	}

	return false;
}

bool DataInspector::InspectVec3(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName)
{
	//const static char* cacheNameFmt = "editor_InspectVec3_{}";

	auto& vec = variant.As<Vec3>();
	auto name = "## " + String(propertyName);
	if (ImGui::DragFloat3(name.c_str(), &vec[0], 0.01f, -INFINITY, INFINITY))
	{
		auto input = Variant::Of<Vec3>();
		input.As<Vec3>() = vec;
		accessor.Set(input);

		return true;
	}

	return false;
}

bool DataInspector::InspectProjectionMat4(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName)
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

	return modified;
}

bool DataInspector::InspectString(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName)
{
	auto& path = variant.AsString();
	ImGui::LabelText("##label", path.c_str());
	return false;
}

bool DataInspector::InspectStringPathEx(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName,
	bool allowOutsideResources, float width, bool directory, const String& startPath, bool directOpenSystemDialog)
{
	bool clicked = false;
	if (!directOpenSystemDialog)
	{
		auto path = variant.AsString();
		ImGui::PushStyleVar(ImGuiStyleVar_ButtonTextAlign, ImVec2(0.0f, 0.5f));

		auto labelName = path + "##" + propertyName;

		clicked = ImGui::Button(labelName.c_str(), ImVec2(width <= 0 ? ImGui::GetWindowWidth() * 0.8f : width, 0));

		ImGui::PopStyleVar();

		ImGui::SameLine();

		auto labelName2 = String("...") + "##" + propertyName;
		clicked = ImGui::Button(labelName2.c_str(), ImVec2(30, 0)) || clicked;
	}

	if (clicked || directOpenSystemDialog)
	{
		std::wstring initDir = L"";
		std::wstring initFilename = L"";
		if (!startPath.empty())
		{
			if (startPath[startPath.length() - 1] == '/')
			{
				initDir = StringUtils::StringToWString(startPath.c_str());
			}
			else
			{
				auto fileName = FileUtils::GetLastName(startPath.c_str());
				initFilename = StringUtils::StringToWString(fileName.c_str());
				initDir = StringUtils::StringToWString(FileUtils::PopPath(startPath).c_str());
			}
		}

#ifdef _WIN32
		if (!directory)
		{
			OPENFILENAME ofn;
			TCHAR Filestring[MAX_PATH] = { 0 };

			ZeroMemory(&ofn, sizeof(ofn));
			ofn.lStructSize = sizeof(ofn);
			ofn.lpstrFile = Filestring;
			ofn.nMaxFile = sizeof(Filestring);
			ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
			ofn.nFilterIndex = 1;
			ofn.lpstrFileTitle = initFilename.empty() ? NULL : initFilename.data();
			ofn.nMaxFileTitle = initFilename.size();
			ofn.lpstrInitialDir = initDir.empty() ? NULL : initDir.c_str();
			ofn.Flags = OFN_NOCHANGEDIR;//OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

			if (GetOpenFileName(&ofn) == TRUE)
			{
				//std::wcout << ofn.lpstrFile << "\n";

				std::wstring_view wstr = ofn.lpstrFile;

				int size_needed = WideCharToMultiByte(CP_UTF8, 0, &ofn.lpstrFile[0], (int)wstr.length(), NULL, 0, NULL, NULL);
				std::string fullPath(size_needed, 0);
				WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &fullPath[0], size_needed, NULL, NULL);

				std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

				auto input = Variant(VARIANT_TYPE::STRING_PATH);
				if (allowOutsideResources)
				{
					auto rcpath = FileSystem::Get()->GetExecutablePath();
					if (fullPath.find(rcpath.c_str()) != 0)
					{
						input.As<String>() = fullPath.c_str();
						accessor.Set(input);
						return true;
					}

					auto rpath = fullPath.substr(rcpath.length());
					input.As<String>() = rpath.c_str();
					accessor.Set(input);
					return true;
				}

				auto rcpath = FileSystem::Get()->GetResourcesRootPath();

				if (fullPath.find(rcpath.c_str()) != 0)
				{
					std::cerr << "Resources must be placed under \"" << rcpath << "\"\n";
					return false;
				}

				auto rpath = fullPath.substr(rcpath.length());
				input.As<String>() = rpath.c_str();
				accessor.Set(input);

				return true;
			}
		}
		
		if (directory)
		{
			//TCHAR szDir[MAX_PATH];
			//BROWSEINFO bInfo;
			//bInfo.hwndOwner = nullptr;
			//bInfo.pidlRoot = NULL;
			//bInfo.pszDisplayName = szDir; // Address of a buffer to receive the display name of the folder selected by the user
			//bInfo.lpszTitle = L"Select a folder"; // Title of the dialog
			//bInfo.ulFlags = 0;
			//bInfo.lpfn = NULL;
			//bInfo.lParam = 0;
			//bInfo.iImage = -1;

			//LPITEMIDLIST lpItem = SHBrowseForFolder(&bInfo);
			//if (lpItem != NULL)
			//{
			//	SHGetPathFromIDList(lpItem, szDir);

			//	std::wstring_view wstr = szDir;
			//	int size_needed = WideCharToMultiByte(CP_UTF8, 0, &szDir[0], (int)wstr.length(), NULL, 0, NULL, NULL);
			//	std::string fullPath(size_needed, 0);
			//	WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &fullPath[0], size_needed, NULL, NULL);

			//	auto input = Variant(VARIANT_TYPE::STRING_PATH);
			//	input.As<String>() = fullPath.c_str();
			//	accessor.Set(input);
			//}

			//std::cout << "OpenDirectory: " << Thread::GetID() << "\n";

			LPWSTR path = nullptr;

			IFileDialog* pfd;
			if (SUCCEEDED(CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pfd))))
			{
				DWORD dwOptions;
				if (SUCCEEDED(pfd->GetOptions(&dwOptions)))
				{
					pfd->SetOptions(dwOptions | FOS_PICKFOLDERS);
				}
				if (SUCCEEDED(pfd->Show(NULL)))
				{
					IShellItem* psi;
					if (SUCCEEDED(pfd->GetResult(&psi)))
					{
						if (!SUCCEEDED(psi->GetDisplayName(SIGDN_DESKTOPABSOLUTEPARSING, &path)))
						{
							MessageBox(NULL, L"GetIDListName() failed", NULL, NULL);
						}
						psi->Release();
					}
				}
				pfd->Release();
			}

			if (path)
			{
				std::wstring_view wstr = path;
				int size_needed = WideCharToMultiByte(CP_UTF8, 0, &path[0], (int)wstr.length(), NULL, 0, NULL, NULL);
				std::string fullPath(size_needed, 0);
				WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &fullPath[0], size_needed, NULL, NULL);

				const std::filesystem::path base = FileUtils::PopPath(FileSystem::Get()->GetExecutablePath()).c_str();
				const std::filesystem::path p = fullPath.c_str();

				auto input = Variant(VARIANT_TYPE::STRING_PATH);
				input.As<String>() = (std::filesystem::relative(p, base).generic_string() + "/").c_str();
				accessor.Set(input);

				return true;
			}
		}
#endif // WIN32
	}

	return false;
}

bool DataInspector::InspectStringPath(ClassMetadata* metadata, Accessor& accessor, const Variant& variant, const char* propertyName)
{
	return InspectStringPathEx(metadata, accessor, variant, propertyName, false);
}

bool DataInspector::Inspect(ClassMetadata* metadata, Accessor& accessor, const char* propertyName)
{
	auto variant = accessor.Get();
	auto func = s_inspectFunc[variant.Type()];
	if (func)
	{
		return func(metadata, accessor, variant, propertyName);
	}
}

void DataInspector::Initialize()
{
	s_inspectFunc[VARIANT_TYPE::BOOL]						= InspectBool;
	s_inspectFunc[VARIANT_TYPE::FLOAT]						= InspectFloat;
	s_inspectFunc[VARIANT_TYPE::UINT64]						= InspectUint64;
	s_inspectFunc[VARIANT_TYPE::VEC3]						= InspectVec3;
	s_inspectFunc[VARIANT_TYPE::TRANSFORM3D]				= InspectTransform;
	s_inspectFunc[VARIANT_TYPE::PROJECTION_MAT4]			= InspectProjectionMat4;
	s_inspectFunc[VARIANT_TYPE::STRING]						= InspectString;
	s_inspectFunc[VARIANT_TYPE::STRING_PATH]				= InspectStringPath;
}