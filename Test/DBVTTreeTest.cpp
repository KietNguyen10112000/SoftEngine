#include <gtest/gtest.h>

#include "Objects/QueryStructures/DBVTQueryTree.h"
//#include "Math/Math.h"

using namespace soft;

TEST(DBVTQueryTreeTest, Construction)
{
	AABox box1 = { { 0,0,0 }, { 1,1,1 } };
	AABox box2 = { { 0,0,-3 }, { 2,1,1 } };
	AABox box3 = { { 5,0,-1 }, { 2,1,1 } };
	AABox box4 = { { 4,0,5 }, { 1,1,2} };
	AABox box5 = { { 1, 2, 3 }, {5,1,2} };

	AABBQueryStructure* tree = rheap::New<DBVTQueryTree>();
	tree->Add(box1, (void*)1);
	tree->Add(box2, (void*)2);
	tree->Add(box3, (void*)3);
	tree->Add(box4, (void*)4);
	tree->Add(box5, (void*)5);

	std::Vector<void*> ret = {};
	tree->QueryAABox(box1.MakeJointed(box2).MakeJointed(box3), ret);

	rheap::Delete(tree);
}