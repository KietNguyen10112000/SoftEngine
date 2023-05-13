#pragma once

#include "Core/TypeDef.h"

#include "Math/Math.h"

#include "../Collision/Collision2DResult.h"

NAMESPACE_BEGIN

inline Rect AARectToRect(const AARect& aaRect)
{
	auto& dimensions = aaRect.m_dimensions;
	Vec2 v1 = Vec2(dimensions.x, 0);
	Vec2 v2 = Vec2(0, dimensions.y);

	Rect ret;
	ret.m_point = aaRect.m_topLeft;
	ret.m_vec1 = v1;
	ret.m_vec2 = v2;
	return ret;
}

inline AARect RectToAARect(const Rect& rect)
{
	Vec2 temp[4];
	rect.GetPoints(temp);
	
	AARect ret;
	ret.FromPoints(temp, 4);
	return ret;
}

template <size_t N_VERTICES>
inline void FindMinMaxProjection(const Vec2& axis, Vec2* vertices, 
	int& minIdx, float& min, int& maxIdx, float& max)
{
	minIdx = 0;
	min = FLT_MAX;
	maxIdx = 0;
	max = -FLT_MAX;

	for (size_t i = 0; i < N_VERTICES; i++)
	{
		auto v = vertices[i].Dot(axis);
		if (min > v)
		{
			minIdx = i;
			min = v;
		}

		if (max < v)
		{
			maxIdx = i;
			max = v;
		}
	}

	assert(maxIdx != minIdx);
}

inline void RectRectCollision(const Rect& A, const Rect& B, Collision2DResult& result)
{
	auto centerA = A.Center();
	auto centerB = B.Center();
	Vec2 temp[4];

	float overlapA1, overlapA2, overlapB1, overlapB2;
	Vec2 nA1, nA2, nB1, nB2;

	{
		// B projects over A axises

		auto& axisA1 = A.m_vec1;
		auto& axisA2 = A.m_vec2;

		B.GetPoints(temp);

		//float centerA1 = axisA1.Dot(centerA);
		float minA1 = axisA1.Dot(A.m_point);
		float maxA1 = axisA1.Dot(A.m_point + A.m_vec1);
		float minA2 = axisA2.Dot(A.m_point);
		float maxA2 = axisA2.Dot(A.m_point + A.m_vec2);

		//float centerB1 = axisA1.Dot(centerB);

		int min1Idx; float min1; int max1Idx; float max1;
		FindMinMaxProjection<4>(axisA1, temp, min1Idx, min1, max1Idx, max1);

		int min2Idx; float min2; int max2Idx; float max2;
		FindMinMaxProjection<4>(axisA2, temp, min2Idx, min2, max2Idx, max2);

		// overlap over axis 1
		auto aExtent1 = (maxA1 - minA1) / 2.0f;
		auto aCenter1 = (maxA1 + minA1) / 2.0f;
		auto bExtent1 = (max1 - min1) / 2.0f;
		auto bCenter1 = (max1 + min1) / 2.0f;
		overlapA1 = aExtent1 + bExtent1 - std::abs(aCenter1 - bCenter1);
		nA1 = axisA1.Normal();

		// overlap over axis 2
		auto aExtent2 = (maxA2 - minA2) / 2.0f;
		auto aCenter2 = (maxA2 + minA2) / 2.0f;
		auto bExtent2 = (max2 - min2) / 2.0f;
		auto bCenter2 = (max2 + min2) / 2.0f;
		overlapA2 = aExtent2 + bExtent2 - std::abs(aCenter2 - bCenter2);
		nA2 = axisA2.Normal();
	}

	{
		// A projects over B axises

		auto& axisB1 = B.m_vec1;
		auto& axisB2 = B.m_vec2;

		A.GetPoints(temp);

		float minB1 = axisB1.Dot(B.m_point);
		float maxB1 = axisB1.Dot(B.m_point + B.m_vec1);
		float minB2 = axisB2.Dot(B.m_point);
		float maxB2 = axisB2.Dot(B.m_point + B.m_vec2);

		int min1Idx; float min1; int max1Idx; float max1;
		FindMinMaxProjection<4>(axisB1, temp, min1Idx, min1, max1Idx, max1);

		int min2Idx; float min2; int max2Idx; float max2;
		FindMinMaxProjection<4>(axisB2, temp, min2Idx, min2, max2Idx, max2);

		auto bExtent1 = (maxB1 - minB1) / 2.0f;
		auto bCenter1 = (maxB1 + minB1) / 2.0f;
		auto aExtent1 = (max1 - min1) / 2.0f;
		auto aCenter1 = (max1 + min1) / 2.0f;
		overlapB1 = aExtent1 + bExtent1 - std::abs(aCenter1 - bCenter1);
		nB1 = axisB1.Normal();

		auto bExtent2 = (maxB2 - minB2) / 2.0f;
		auto bCenter2 = (maxB2 + minB2) / 2.0f;
		auto aExtent2 = (max2 - min2) / 2.0f;
		auto aCenter2 = (max2 + min2) / 2.0f;
		overlapB2 = aExtent2 + bExtent2 - std::abs(aCenter2 - bCenter2);
		nB2 = axisB2.Normal();
	}
	
	auto overlapMin = std::min(overlapA1, std::min(overlapA2, std::min(overlapB1, overlapB2)));
	result.penetration = overlapMin;

	if (overlapMin < 0)
	{
		result.penetration = 0;
		return;
	}

	if (overlapMin == overlapA1)
	{
		result.AB = nA1;
		goto Return;
	}

	if (overlapMin == overlapA2)
	{
		result.AB = nA2;
		goto Return;
	}

	if (overlapMin == overlapB1)
	{
		result.AB = nB1;
		goto Return;
	}

	if (overlapMin == overlapB2)
	{
		result.AB = nB2;
		goto Return;
	}

Return:
	auto cAB = (centerB - centerA);
	if (result.AB.Dot(cAB) < 0)
	{
		result.AB = -result.AB;
	}
	
}

NAMESPACE_END