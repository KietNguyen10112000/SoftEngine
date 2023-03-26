#include "DBVTQueryTree.h"

#include "Core/Memory/Memory.h"


NAMESPACE_BEGIN

class DBVTQueryTreeSession : public AABBQuerySession
{
public:
    constexpr static size_t INIT_CAPACITY = 16 * KB;
    std::Vector<DBVTQueryTree::NodeId> m_stack;

public:
    DBVTQueryTreeSession()
    {
        m_stack.reserve(INIT_CAPACITY);
        m_result.reserve(INIT_CAPACITY);
    }

    ~DBVTQueryTreeSession()
    {

    }

};

DBVTQueryTree::DBVTQueryTree()
{
    m_nodes.resize(1024);
}

DBVTQueryTree::~DBVTQueryTree()
{
    /*if (m_root)
    {
        FreeTree(m_root);
    }

    if (m_allocatedNode)
    {
        while (m_allocatedNode)
        {
            auto next = m_allocatedNode->next;
            rheap::Delete(m_allocatedNode);
            m_allocatedNode = next;
        }
    }*/
}

DBVTQueryTree::NodeId DBVTQueryTree::AllocateNode()
{
    NodeId ret;
    if (m_allocatedNode != INVALID_ID)
    {
        ret = m_allocatedNode;
        m_allocatedNode = Get(ret).next;
        goto Return;
    }

    if (m_nodesAllocatedCount == m_nodes.size())
    {
        m_nodes.resize(m_nodes.size() * 2);
    }

    ret = m_nodesAllocatedCount++;

Return:
    Get(ret).parent = INVALID_ID;
    Get(ret).child1 = INVALID_ID;
    Get(ret).child2 = INVALID_ID;
    Get(ret).height = 0;

    return ret;
}

void DBVTQueryTree::DeallocateNode(DBVTQueryTree::NodeId node)
{
    Get(node).next = m_allocatedNode;
    m_allocatedNode = node;
}

void DBVTQueryTree::FreeTree(Node* node)
{
    /*Node* it = node;
    while (it != nullptr)
    {
        if (it->IsLeaf())
        {
            rheap::Delete(it);
            auto parent = it->parent;

            if (!parent)
            {
                break;
            }

            if (parent->child1 == it)
            {
                parent->child1 = nullptr;
            }
            else
            {
                parent->child2 = nullptr;
            }

            it = parent;
            continue;
        }

        it = it->child2 ? it->child2 : it->child1;
    }*/
}

#define JOINT_2_NODES(root, node)           \
Node* jointedNode = AllocateNode();         \
jointedNode->bound = root->bound;           \
jointedNode->bound.Joint(node->bound);      \
                                            \
auto parent = root->parent;                 \
                                            \
jointedNode->child1 = root;                 \
jointedNode->child2 = node;                 \
jointedNode->parent = parent;               \
                                            \
root->parent = jointedNode;                 \
node->parent = jointedNode;                 \
                                            \
if (parent)                                 \
{                                           \
    if (parent->child1 == root)             \
    {                                       \
        parent->child1 = jointedNode;       \
    }                                       \
    else                                    \
    {                                       \
        parent->child2 = jointedNode;       \
    }                                       \
}                                           \
else                                        \
{                                           \
    m_root = jointedNode;                   \
}

DBVTQueryTree::NodeId DBVTQueryTree::Balance(NodeId node)
{
    NodeId ret = node;

    auto parent = Get(node).parent;
    auto child1 = Get(node).child1;
    auto child2 = Get(node).child2;

    intmax_t dh12 = Get(child1).height - Get(child2).height;

    NodeId target = INVALID_ID;
    //Node* lowerChild = nullptr;
    if (dh12 > 1)
    {
        //lowerChild = child2;

        if (Get(Get(child1).child1).height > Get(Get(child1).child2).height)
        {
            target = Get(child1).child2;
            Get(child1).child2 = node;
        }
        else
        {
            target = Get(child1).child1;
            Get(child1).child1 = node;
        }

        Get(node).parent = child1;
        Get(child1).parent = parent;

        if (parent != INVALID_ID)
        {
            if (Get(parent).child1 == node)
            {
                Get(parent).child1 = child1;
            }
            else
            {
                Get(parent).child2 = child1;
            }
        }
        else
        {
            m_root = child1;
        }

        Get(node).child1 = target;
        Get(target).parent = node;

        ret = child2;
    }

    if (dh12 < -1)
    {
        //lowerChild = child1;

        if (Get(Get(child2).child1).height > Get(Get(child2).child2).height)
        {
            target = Get(child2).child2;
            Get(child2).child2 = node;
        }
        else
        {
            target = Get(child2).child1;
            Get(child2).child1 = node;
        }

        Get(node).parent = child2;
        Get(child2).parent = parent;

        if (parent != INVALID_ID)
        {
            if (Get(parent).child1 == node)
            {
                Get(parent).child1 = child2;
            }
            else
            {
                Get(parent).child2 = child2;
            }
        }
        else
        {
            m_root = child2;
        }

        Get(node).child2 = target;
        Get(target).parent = node;

        ret = child1;
    }

    return ret;
}

void DBVTQueryTree::AddNode(NodeId node)
{
    //size_t count = 0;

    // find the best node for insertion
    NodeId it = m_root;
    while (true)
    {
        if (Get(it).IsLeaf())
        {
            break;
        }

        // choose the better side to recursive call
        auto child1 = Get(it).child1;
        auto child2 = Get(it).child2;

        // distance from node to childs bound
        auto d1 = (Get(child1).bound.GetCenter() - Get(node).bound.GetCenter()).Length2();
        auto d2 = (Get(child2).bound.GetCenter() - Get(node).bound.GetCenter()).Length2();

        // self distance
        auto d12 = (Get(child1).bound.GetCenter() - Get(child2).bound.GetCenter()).Length2();

        auto min = std::min(std::min(d1, d2), d12);

        if (min == d1)
        {
            it = child1;
            //count++;
            continue;
        }

        if (min == d2)
        {
            it = child2;
            //count++;
            continue;
        }

        break;
    }

    //std::cout << "Insert depth: " << count << "\n";

    // insert node to the best node
    NodeId jointedNode = AllocateNode();
    Get(jointedNode).bound = Get(it).bound;
    Get(jointedNode).bound.Joint(Get(node).bound);

    auto parent = Get(it).parent;

    Get(jointedNode).child1 = it;
    Get(jointedNode).child2 = node;
    Get(jointedNode).parent = parent;

    Get(it).parent = jointedNode;
    Get(node).parent = jointedNode;

    if (parent != INVALID_ID)
    {
        if (Get(parent).child1 == it)
        {
            Get(parent).child1 = jointedNode;
        }
        else
        {
            Get(parent).child2 = jointedNode;
        }
    }
    else
    {
        m_root = jointedNode;
    }

    // walk back to fix height
    it = jointedNode;
    while (it != INVALID_ID)
    {
        it = Balance(it);

        if (Get(it).IsLeaf())
        {
            Get(it).height = 0;
        }
        else
        {
            Get(it).height = std::max(Get(Get(it).child1).height, Get(Get(it).child2).height) + 1;
            Get(it).bound = Get(Get(it).child1).bound.MakeJointed(Get(Get(it).child2).bound);
        }

        it = Get(it).parent;
    }
}

void DBVTQueryTree::RemoveNode(NodeId node)
{
    assert(Get(node).IsLeaf());

    auto parent = Get(node).parent;
    auto grandParent = Get(parent).parent;
    auto sibling = Get(parent).child1 == node ? Get(parent).child2 : Get(parent).child1;

    if (parent == INVALID_ID)
    {
        m_root = INVALID_ID;
        goto Return;
    }

    if (grandParent == INVALID_ID)
    {
        m_root = sibling;
        Get(sibling).parent = INVALID_ID;
        goto Return;
    }

    if (Get(grandParent).child1 == parent)
    {
        Get(grandParent).child1 = sibling;
    }
    else
    {
        Get(grandParent).child2 = sibling;
    }

    Get(sibling).parent = grandParent;

    auto it = grandParent;
    while (it != INVALID_ID)
    {
        it = Balance(it);

        if (Get(it).IsLeaf())
        {
            Get(it).height = 0;
        }
        else
        {
            Get(it).height = std::max(Get(Get(it).child1).height, Get(Get(it).child2).height) + 1;
            Get(it).bound = Get(Get(it).child1).bound.MakeJointed(Get(Get(it).child2).bound);
        }

        it = Get(it).parent;
    }

Return:
    DeallocateNode(parent);
    DeallocateNode(node);
}

size_t DBVTQueryTree::Height(NodeId node)
{
    if (node == INVALID_ID || Get(node).IsLeaf()) return 0;

    auto h1 = Height(Get(node).child1);
    auto h2 = Height(Get(node).child2);

    return std::max(h1, h2) + 1;
}

size_t DBVTQueryTree::Height()
{
    return Height(m_root);
}


void DBVTQueryTree::Validate(NodeId node)
{
    if (Get(node).IsLeaf())
    {
        if (Get(node).height != 0)
        {
            std::cout << "[ERROR]:\tDBVTQueryTree::Validate failed\n";
            assert(0);
        }
        return;
    }

    intmax_t d = Get(Get(node).child1).height - Get(Get(node).child2).height;
    if (d > 1 || d < -1)
    {
        std::cout << "[ERROR]:\tDBVTQueryTree::Validate failed\n";
        assert(0);
    }

    Validate(Get(node).child1);
    Validate(Get(node).child2);
}


#undef JOINT_2_NODES

ID DBVTQueryTree::Add(const AABox& aabb, void* userPtr)
{
    NodeId newNode = AllocateNode();
    Get(newNode).bound = aabb;
    Get(newNode).userPtr = userPtr;

    if (m_root == INVALID_ID)
    {
        m_root = newNode;
    }
    else
    {
        AddNode(newNode);
    }

//#ifdef _DEBUG
//    Validate(m_root);
//#endif // _DEBUG

    return (ID)newNode;
}

void DBVTQueryTree::Remove(ID id)
{
    // assert id must be leaf node
    RemoveNode((NodeId)id);

//#ifdef _DEBUG
//    Validate(m_root);
//#endif // _DEBUG
}

void DBVTQueryTree::Clear()
{
    m_allocatedNode = INVALID_ID;
    m_nodesAllocatedCount = 0;
    m_root = INVALID_ID;
}

AABBQuerySession* DBVTQueryTree::NewSession()
{
    return rheap::New<DBVTQueryTreeSession>();
}

void DBVTQueryTree::DeleteSession(AABBQuerySession* session)
{
    return rheap::Delete((DBVTQueryTreeSession*)session);
}

void DBVTQueryTree::QueryAABox(const AABox& aabox, AABBQuerySession* session)
{
    if (m_root != INVALID_ID && Get(m_root).bound.IsOverlap(aabox))
    {
        auto* dvbtSession = (DBVTQueryTreeSession*)session;
        auto& stack = dvbtSession->m_stack;
        auto& result = dvbtSession->m_result;

        stack.push_back(m_root);

        while (!stack.empty())
        {
            auto node = stack.back();
            stack.pop_back();

            if (Get(node).IsLeaf())
            {
                result.push_back(Get(node).userPtr);
                continue;
            }

            if (Get(Get(node).child1).bound.IsOverlap(aabox))
            {
                stack.push_back(Get(node).child1);
            }

            if (Get(Get(node).child2).bound.IsOverlap(aabox))
            {
                stack.push_back(Get(node).child2);
            }
        }
    }
}

void DBVTQueryTree::QueryFrustum(const Frustum& frustum, AABBQuerySession* session)
{
    if (m_root != INVALID_ID && frustum.IsOverlap(Get(m_root).bound))
    {
        auto* dvbtSession = (DBVTQueryTreeSession*)session;
        auto& stack = dvbtSession->m_stack;
        auto& result = dvbtSession->m_result;

        stack.push_back(m_root);

        while (!stack.empty())
        {
            auto node = stack.back();
            stack.pop_back();

            if (Get(node).IsLeaf())
            {
                result.push_back(Get(node).userPtr);
                continue;
            }

            if (frustum.IsOverlap(Get(Get(node).child1).bound))
            {
                stack.push_back(Get(node).child1);
            }

            if (frustum.IsOverlap(Get(Get(node).child2).bound))
            {
                stack.push_back(Get(node).child2);
            }
        }
    }
}

void DBVTQueryTree::QuerySphere(const Sphere& sphere, AABBQuerySession* session)
{
}

void DBVTQueryTree::QueryBox(const Box& box, AABBQuerySession* session)
{
}

void DBVTQueryTree::Query(AABBQueryTester* tester, AABBQuerySession* session)
{
}


NAMESPACE_END