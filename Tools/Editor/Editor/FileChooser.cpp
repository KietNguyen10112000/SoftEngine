#include "FileChooser.h"

#include "FileSystem/FileSystem.h"
#include "Common/Base/Variant.h"

#ifdef _WIN32
#define NOMINMAX
#include <Windows.h>
#undef near
#undef far
#endif

String FileChooser::OpenFileChooser(const String& filterExtensions, bool allowOutsideResources)
{
#ifdef _WIN32
	OPENFILENAME ofn;
	TCHAR Filestring[MAX_PATH] = { 0 };

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = Filestring;
	ofn.nMaxFile = sizeof(Filestring);
	ofn.lpstrFilter = L"All\0*.*\0Text\0*.TXT\0";
	ofn.nFilterIndex = 1;
	ofn.lpstrFileTitle = NULL;
	ofn.nMaxFileTitle = 0;
	ofn.lpstrInitialDir = NULL;
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
				return fullPath.c_str();
			}

			auto rpath = fullPath.substr(rcpath.length());
			return rcpath;
		}

		auto rcpath = FileSystem::Get()->GetResourcesRootPath();

		if (fullPath.find(rcpath.c_str()) != 0)
		{
			std::cerr << "Resources must be placed under \"" << rcpath << "\"\n";
			return nullptr;
		}

		rcpath = fullPath.substr(rcpath.length()).c_str();
		return rcpath;
	}
#else
	assert(0);
#endif // WIN32
}