#include "SystemDialog.h"

#include "FileSystem/FileSystem.h"
#include "FileSystem/FileUtils.h"

#ifdef _WIN32
#include <Windows.h>
#include <shlobj_core.h>
#undef near
#undef far
#endif

bool SystemDialog::OpenSaveAsDialog(SaveAsDialog& opt)
{
	std::wstring initDir = L"";
	std::wstring initFilename = L"";
	if (!opt.defaultPath.empty())
	{
		if (opt.defaultPath[opt.defaultPath.length() - 1] == '/')
		{
			initDir = StringUtils::StringToWString(opt.defaultPath.c_str());
		}
		else
		{
			auto fileName = FileUtils::GetLastName(opt.defaultPath.c_str());
			initFilename = StringUtils::StringToWString(fileName.SubString(0, fileName.RFind('.')).c_str());
			initDir = StringUtils::StringToWString(FileUtils::PopPath(opt.defaultPath).c_str());
		}
	}

	std::string extensions;
	std::wstring defExtension;
	if (opt.extensionGroups.empty())
	{
		extensions = "All\0*.*\0";
	}
	else
	{
		extensions = "";
		for (size_t i = 0; i < opt.extensionGroups.size(); i++)
		{
			auto& group = opt.extensionGroups[i];
			extensions += group.groupName.c_str();
			extensions += ',';
			for (auto& e : group.extensions)
			{
				extensions += e.c_str(); 
				extensions += ',';
			}
		}

		defExtension = StringUtils::StringToWString(opt.extensionGroups[0].extensions[0].c_str());
	}

	std::wstring wextensions = StringUtils::StringToWString(extensions);
	std::replace(wextensions.begin(), wextensions.end(), L',', L'\0');
	wextensions.push_back(L'\0');

#ifdef WIN32
	OPENFILENAME ofn;
	TCHAR Filestring[MAX_PATH] = { 0 };

	if (!initFilename.empty())
	{
		std::memcpy(Filestring, initFilename.data(), initFilename.size() * sizeof(wchar_t));
	}

	ZeroMemory(&ofn, sizeof(ofn));
	ofn.lStructSize = sizeof(ofn);
	ofn.lpstrFile = Filestring;
	ofn.nMaxFile = sizeof(Filestring);
	ofn.lpstrFilter = wextensions.c_str();
	ofn.lpstrDefExt = defExtension.empty() ? NULL : defExtension.c_str();
	ofn.nFilterIndex = 1;
	//ofn.lpstrFileTitle = initFilename.empty() ? NULL : initFilename.data();
	//ofn.nMaxFileTitle = initFilename.size();
	ofn.lpstrInitialDir = initDir.empty() ? NULL : initDir.c_str();
	ofn.Flags = OFN_NOCHANGEDIR;//OFN_PATHMUSTEXIST | OFN_FILEMUSTEXIST;

	if (GetSaveFileName(&ofn) == TRUE)
	{
		//std::wcout << ofn.lpstrFile << "\n";

		std::wstring_view wstr = ofn.lpstrFile;

		int size_needed = WideCharToMultiByte(CP_UTF8, 0, &ofn.lpstrFile[0], (int)wstr.length(), NULL, 0, NULL, NULL);
		std::string fullPath(size_needed, 0);
		WideCharToMultiByte(CP_UTF8, 0, &wstr[0], (int)wstr.size(), &fullPath[0], size_needed, NULL, NULL);

		std::replace(fullPath.begin(), fullPath.end(), '\\', '/');

		auto& rcpath = FileSystem::Get()->GetExecutablePath();
		if (fullPath.find(rcpath.c_str()) != 0)
		{
			opt.outputFilePath = fullPath.c_str();
			return true;
		}

		opt.outputFilePath = fullPath.substr(rcpath.length()).c_str();
		return true;
	}

	return false;
#else
	assert(0);
#endif // WIN32
}
