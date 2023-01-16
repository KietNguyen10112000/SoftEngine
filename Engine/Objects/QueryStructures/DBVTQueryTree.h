#pragma once

#include "AABBQueryStructure.h"

NAMESPACE_BEGIN


class API DBVTQueryTree : public AABBQueryStructure
{
public:
	struct Node
	{
		Node* parent = nullptr;
		Node* left = nullptr;
		Node* right = nullptr;
		AABox bound;
		void* userPtr;

		inline bool IsLeaf() const
		{
#ifdef _DEBUG
			if (left == nullptr) assert(right == nullptr);
#endif

			return left == nullptr;
		}
	};

protected:
	Node* m_root = nullptr;

public:
	DBVTQueryTree();
	~DBVTQueryTree();

protected:
	void FreeTree(Node* node);
	void AddNode(Node* root, Node* node);
	void QueryAABoxRecursive(Node* node, const AABox& aabox, std::Vector<void*>& output);
	
public:
	// Inherited via AABBQueryStructure
	virtual ID Add(const AABox& aabb, void* userPtr) override;

	virtual void Remove(ID id) override;

	virtual void QueryAABox(const AABox& aabox, std::Vector<void*>& output) override;

	virtual void QuerySphere(const Sphere& sphere, std::Vector<void*>& output) override;

	virtual void QueryBox(const Box& box, std::Vector<void*>& output) override;

	virtual void Query(AABBQueryTester* tester, std::Vector<void*>& output) override;

};


NAMESPACE_END