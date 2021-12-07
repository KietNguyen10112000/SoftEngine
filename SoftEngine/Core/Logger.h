#pragma once

#include <Math/Math.h>

#include <string>

inline std::wstring ToString(const Mat4x4& mat)
{
	std::wstring ret = L"[";

	for (size_t i = 0; i < 4; i++)
	{
		ret += L'[';
		for (size_t j = 0; j < 4; j++)
		{
			ret += std::to_wstring(mat.m[i][j]) + L"\t";
		}
		ret += L"]\n ";
	}

	ret.pop_back();
	ret.pop_back();

	return ret + L"]";
}

inline std::wstring ToString(const Vec3& vec)
{
	std::wstring ret = L"[" + std::to_wstring(vec.x) +
		L'\t' + std::to_wstring(vec.y) + L'\t'+ std::to_wstring(vec.z) + L"]";

	return ret;
}