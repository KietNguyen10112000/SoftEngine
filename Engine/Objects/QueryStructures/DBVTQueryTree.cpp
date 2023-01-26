#include "DBVTQueryTree.h"

#include "Core/Memory/Memory.h"


NAMESPACE_BEGIN

class DBVTQueryTreeSession : public AABBQuerySession
{
public:
    constexpr static size_t INIT_CAPACITY = 16 * KB;
    std::Vector<DBVTQueryTree::Node*> m_stack;

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

}

DBVTQueryTree::~DBVTQueryTree()
{
    if (m_root)
    {
        FreeTree(m_root);
    }
}

void DBVTQueryTree::FreeTree(Node* node)
{
    Node* it = node;
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

            if (parent->left == it)
            {
                parent->left = nullptr;
            }
            else
            {
                parent->right = nullptr;
            }

            it = parent;
            continue;
        }

        it = it->right ? it->right : it->left;
    }
}

#define JOINT_2_NODES(root, node)           \
Node* jointedNode = rheap::New<Node>();     \
jointedNode->bound = root->bound;           \
jointedNode->bound.Joint(node->bound);      \
                                            \
auto parent = root->parent;                 \
                                            \
jointedNode->left = root;                   \
jointedNode->right = node;                  \
jointedNode->parent = parent;               \
                                            \
root->parent = jointedNode;                 \
node->parent = jointedNode;                 \
                                            \
if (parent)                                 \
{                                           \
    if (parent->left == root)               \
    {                                       \
        parent->left = jointedNode;         \
    }                                       \
    else                                    \
    {                                       \
        parent->right = jointedNode;        \
    }                                       \
}                                           \
else                                        \
{                                           \
    m_root = jointedNode;                   \
}

void DBVTQueryTree::AddNode(Node* root, Node* node)
{
    Node* it = root;
    while (it != nullptr)
    {
        if (it->IsLeaf())
        {
            JOINT_2_NODES(it, node);
            return;
        }

        // choose the better side to recursive call
        auto left = it->left;
        auto right = it->right;

        // distance from node to childs bound
        auto dl = (left->bound.GetCenter() - node->bound.GetCenter()).Length2();
        auto dr = (right->bound.GetCenter() - node->bound.GetCenter()).Length2();

        // self distance
        auto ds = (left->bound.GetCenter() - right->bound.GetCenter()).Length2();

        auto min = std::min(std::min(dl, dr), ds);

        if (min == dl)
        {
            //AddNode(left, node);
            it = left;
            continue;
        }

        if (min == dr)
        {
            //AddNode(right, node);
            it = right;
            continue;
        }

        JOINT_2_NODES(it, node);
        return;
    }
}

#undef JOINT_2_NODES

ID DBVTQueryTree::Add(const AABox& aabb, void* userPtr)
{
    Node* newNode = rheap::New<Node>();
    newNode->bound = aabb;
    newNode->userPtr = userPtr;

    if (m_root == nullptr)
    {
        m_root = newNode;
    }
    else
    {
        AddNode(m_root, newNode);
    }

    return (ID)newNode;
}

void DBVTQueryTree::Remove(ID id)
{
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
    if (m_root && m_root->bound.IsOverlap(aabox))
    {
        auto* dvbtSession = (DBVTQueryTreeSession*)session;
        auto& stack = dvbtSession->m_stack;
        auto& result = dvbtSession->m_result;

        stack.push_back(m_root);

        while (!stack.empty())
        {
            auto node = stack.back();
            stack.pop_back();

            if (node->IsLeaf())
            {
                result.push_back(node->userPtr);
                continue;
            }

            if (node->right->bound.IsOverlap(aabox))
            {
                stack.push_back(node->right);
            }

            if (node->left->bound.IsOverlap(aabox))
            {
                stack.push_back(node->left);
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