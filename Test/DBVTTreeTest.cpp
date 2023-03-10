#include <gtest/gtest.h>

#include "Core/Random/Random.h"
#include "Core/Time/Clock.h"
#include "Core/Structures/String.h"

#include "Objects/QueryStructures/DBVTQueryTree.h"

#include "GTestLogger.h"

//#include "Math/Math.h"

using namespace soft;

//TEST(DBVTQueryTreeTest, Construction)
//{
//	AABox box1 = { { 0,0,0 }, { 1,1,1 } };
//	AABox box2 = { { 0,0,-3 }, { 2,1,1 } };
//	AABox box3 = { { 5,0,-1 }, { 2,1,1 } };
//	AABox box4 = { { 4,0,5 }, { 1,1,2} };
//	AABox box5 = { { 1, 2, 3 }, {5,1,2} };
//
//	AABBQueryStructure* tree = rheap::New<DBVTQueryTree>();
//	tree->Add(box1, (void*)1);
//	tree->Add(box2, (void*)2);
//	tree->Add(box3, (void*)3);
//	tree->Add(box4, (void*)4);
//	tree->Add(box5, (void*)5);
//
//	auto session = tree->NewSession();
//	tree->QueryAABox(box1.MakeJointed(box2).MakeJointed(box3), session);
//
//	tree->DeleteSession(session);
//	rheap::Delete(tree);
//}

//TEST(DBVTQueryTreeTest, Query)
//{
//	constexpr float rangeX = 1000;
//	constexpr float rangeY = 1000;
//	constexpr float rangeZ = 1000;
//
//	constexpr float rangeDimX = 100;
//	constexpr float rangeDimY = 100;
//	constexpr float rangeDimZ = 100;
//	constexpr size_t num   = 3000;
//
//	for (size_t j = 0; j < 20; j++)
//	{
//		AABBQueryStructure* tree = rheap::New<DBVTQueryTree>();
//
//		auto start = Clock::ns::now();
//		for (size_t i = 0; i < (j == 0 ? 1000'00 : num); i++)
//		{
//			AABox aabb = {
//				Vec3(
//					Random::RangeFloat(-rangeX, rangeX),
//					Random::RangeFloat(-rangeY, rangeY),
//					Random::RangeFloat(-rangeZ, rangeZ)
//				),
//
//				Vec3(
//					Random::RangeFloat(1, rangeDimX),
//					Random::RangeFloat(1, rangeDimY),
//					Random::RangeFloat(1, rangeDimZ)
//				),
//			};
//
//			tree->Add(aabb, (void*)i);
//		}
//		auto dt = (Clock::ns::now() - start) / 1000000;
//		GTestLogger::Log(String::Format("Taken time {} ms ...", dt));
//
//		//std::Vector<void*> ret = {};
//
//		rheap::Delete(tree);
//	}
//	
//}

TEST(DBVTQueryTreeTest, Query2)
{
	constexpr float rangeX = 1000;
	constexpr float rangeY = 1000;
	constexpr float rangeZ = 1000;

	constexpr float rangeDimX = 100;
	constexpr float rangeDimY = 100;
	constexpr float rangeDimZ = 100;
	constexpr size_t num = 5000;

	std::Vector<ID> objs = {};
	std::Vector<AABox> objsAABB = {};
	std::Vector<ID> removes = {};
	DBVTQueryTree* tree = rheap::New<DBVTQueryTree>();

	//size_t seed = 1647024763949200;//Clock::ns::now();
	//size_t seeds[32] = {};
	//for (size_t i = 0; i < 32; i++)
	//{
	//	seeds[i] = seed;
	//}

	//GTestLogger::Log(String::Format("Random seed: {}", seed));

	//Random::Initialize(seeds);

	auto start = Clock::ns::now();
	for (size_t i = 0; i < num; i++)
	{
		AABox aabb = {
			Vec3(
				Random::RangeFloat(-rangeX, rangeX),
				Random::RangeFloat(-rangeY, rangeY),
				Random::RangeFloat(-rangeZ, rangeZ)
			),

			Vec3(
				Random::RangeFloat(1, rangeDimX),
				Random::RangeFloat(1, rangeDimY),
				Random::RangeFloat(1, rangeDimZ)
			),
		};

		objs.push_back(tree->Add(aabb, (void*)i));
		objsAABB.push_back(aabb);
	}
	auto dt = (Clock::ns::now() - start) / 1000000;
	GTestLogger::Log(String::Format("Taken time {} ms ...", dt));
	GTestLogger::Log(String::Format("Tree height {}", tree->Height()));

	for (size_t j = 0; j < 100; j++)
	{
		removes.clear();
		start = Clock::ns::now();
		for (size_t i = 0; i < num; i++)
		{
			if (Random::RangeInt64(0, 1) == 0)
			{
				tree->Remove(objs[i]);
				removes.push_back(i);
			}
		}

		for (size_t i = 0; i < removes.size(); i++)
		{
			objs[removes[i]] = tree->Add(objsAABB[removes[i]], (void*)removes[i]);
		}

		auto dt = (Clock::ns::now() - start) / 1000000;
		GTestLogger::Log(String::Format("removes.size() = {}", removes.size()));
		GTestLogger::Log(String::Format("Taken time {} ms ...", dt));
	}
	

	rheap::Delete(tree);

}