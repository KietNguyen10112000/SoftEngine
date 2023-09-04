#pragma once

#include "AABBQueryStructure.h"

NAMESPACE_BEGIN


class API BVH : public AABBQueryStructure
{
public:
	using NodeId = ID;

	struct Node
	{
		NodeId parent = INVALID_ID;
		NodeId child1 = INVALID_ID;
		union
		{
			NodeId child2 = INVALID_ID;
			NodeId next;
			void* userPtr;
		};

		size_t height = 0;

		AABox bound;

		inline bool IsLeaf() const
		{
			return child1 == INVALID_ID;
		}
	};


protected:
	//Node* m_root = nullptr;
	//Node* m_allocatedNode = nullptr;

	std::Vector<Node> m_nodes;
	size_t m_nodesAllocatedCount = 0;
	NodeId m_allocatedNode = INVALID_ID;
	NodeId m_root = INVALID_ID;

public:
	BVH();
	~BVH();

protected:
	NodeId AllocateNode();
	void DeallocateNode(NodeId node);

	NodeId Balance(NodeId node);

	void FreeTree(Node* node);
	void AddNode(NodeId node);
	void RemoveNode(NodeId node);

	size_t Height(NodeId node);

	void Validate(NodeId node);

	/*inline Node& Get(NodeId id)
	{
		return m_nodes[id];
	}*/
	
public:
	size_t Height();

public:
	// Inherited via AABBQueryStructure
	virtual ID Add(const AABox& aabb, void* userPtr) override;

	virtual void Remove(ID id) override;

	virtual void Clear() override;

	virtual AABBQuerySession* NewSession() override;

	virtual void DeleteSession(AABBQuerySession* session) override;

	virtual void QueryAABox(const AABox& aabox, AABBQuerySession* session) override;

	virtual void QuerySphere(const Sphere& sphere, AABBQuerySession* session) override;

	virtual void QueryBox(const Box& box, AABBQuerySession* session) override;

	virtual void QueryFrustum(const Frustum& frustum, AABBQuerySession* session) override;

	virtual void Query(AABBQueryTester* tester, AABBQuerySession* session) override;

};


NAMESPACE_END