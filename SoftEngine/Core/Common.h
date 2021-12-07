#pragma once

#ifdef _WIN32
#include <Windows.h>
#define GetTime() GetTickCount64()
#else
#include <chrono>

inline unsigned long long GetTime()
{
	return std::chrono::high_resolution_clock::now().time_since_epoch().count();
}
#endif // _WIN64

#define SafeDelete(p) if(p) { delete p; p = nullptr; }

#define RowMajorArrayIndex2dTo1d(y, x, array2dWidth) y * array2dWidth + x

#ifdef _DEBUG
#ifndef Throw
#define Throw(msg) throw msg L"\nThrow from File \"" __FILEW__ L"\", Line " _STRINGIZE(__LINE__) L"."
#endif // !Throw
#include <cassert>
#else
#ifndef Throw
#define Throw(msg) throw msg
#endif // !Throw
#endif // DEBUG

#include <string>
#include <vector>
#include <algorithm>
#include <mutex>

template<class Base, class T>
inline bool instanceof(T* ptr) {
	try
	{
		return dynamic_cast<Base*>(ptr) != nullptr;
	}
	catch (const std::exception & e)
	{
		return false;
	}
}

inline void ToLower(std::string& strToConvert)
{
	std::transform(strToConvert.begin(), strToConvert.end(), strToConvert.begin(),
		[](unsigned char c) { return std::tolower(c); });
}

inline void StringSplit(std::string s, std::string delimiter, std::vector<std::string>& out)
{
	out.clear();
	size_t pos = 0;
	std::string token;
	while ((pos = s.find(delimiter)) != std::string::npos) {
		token = s.substr(0, pos);
		if (!token.empty())
			out.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	if (!s.empty()) out.push_back(s);
}

inline void StringSplit(std::wstring s, std::wstring delimiter, std::vector<std::wstring>& out)
{
	out.clear();
	size_t pos = 0;
	std::wstring token;
	while ((pos = s.find(delimiter)) != std::wstring::npos) {
		token = s.substr(0, pos);
		if (!token.empty())
			out.push_back(token);
		s.erase(0, pos + delimiter.length());
	}
	if (!s.empty()) out.push_back(s);
}

inline std::string WStringToString(const std::wstring& wstr)
{
#ifdef _WIN32
	std::string convertedStr('c', wstr.length() * 2);

	int convertResult = WideCharToMultiByte(CP_UTF8, 0, wstr.data(), wstr.length(),
		convertedStr.data(), convertedStr.length(), NULL, NULL);
	if (convertResult >= 0)
	{
		std::string result(&convertedStr[0], &convertedStr[convertResult]);
		//convertedStr[convertResult] = '\0';

		if (result.empty()) return convertedStr;

		return result;
	}
	else
	{
		return "";
	}
#else
	return "";
#endif
}

inline std::wstring StringToWString(const std::string& str)
{
	std::wstring re;
	re.resize(str.size());
	for (size_t i = 0; i < str.length(); i++)
	{
		re[i] = str[i];
	}

	return re;
}

inline int RandomInt(int min, int max)
{
	srand(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	return min + rand() % (max + 1 - min);
}

inline float RandomFloat(float min, float max)
{
	srand(std::chrono::high_resolution_clock::now().time_since_epoch().count());
	float scale = rand() / (float)RAND_MAX; /* [0, 1.0] */
	return min + scale * (max - min);
}

//return 1 on lock occupied
//return 0 in ow
inline int WaitToLock(std::mutex& lock, unsigned long perCheckTime = 16, long long timeout = LLONG_MAX)
{
	long long count = 0;
	while (!lock.try_lock())
	{
		count += perCheckTime;
		Sleep(perCheckTime);
		if (count > timeout)
		{
			return 0;
		}
	}

	return 1;
}

inline void Release(std::mutex& lock)
{
	lock.unlock();
}