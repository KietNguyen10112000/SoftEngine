#include <gtest/gtest.h>

#include "Core/Memory/DeferredBuffer.h"

#include "Math/Math.h"

using namespace soft;

TEST(DeferredBufferTest, Test)
{
	struct Temp
	{
		Vec3 position;
	};

	auto t = DeferredBufferTracker::Get();

	DeferredBuffer<Temp> buffer = {};
	auto accessor = buffer.GetAccessor();

	auto& position = accessor.Get<&Temp::position>();
	accessor.Set<&Temp::position>(position);
}