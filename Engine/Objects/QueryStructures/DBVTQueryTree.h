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
	
public:
	// Inherited via AABBQueryStructure
	virtual ID Add(const AABox& aabb, void* userPtr) override;

	virtual void Remove(ID id) override;

	virtual AABBQuerySession* NewSession() override;

	virtual void DeleteSession(AABBQuerySession* session) override;

	virtual void QueryAABox(const AABox& aabox, AABBQuerySession* session) override;

	virtual void QuerySphere(const Sphere& sphere, AABBQuerySession* session) override;

	virtual void QueryBox(const Box& box, AABBQuerySession* session) override;

	virtual void Query(AABBQueryTester* tester, AABBQuerySession* session) override;

};


NAMESPACE_END