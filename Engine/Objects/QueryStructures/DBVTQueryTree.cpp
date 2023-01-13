#include "DBVTQueryTree.h"

#include "Core/Memory/Memory.h"

NAMESPACE_BEGIN

DBVTQueryTree::DBVTQueryTree()
{

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
    if (root->IsLeaf())
    {
        JOINT_2_NODES(root, node);
        return;
    }

    // choose the better side to recursive call
    auto left = root->left;
    auto right = root->right;

    // distance from node to childs bound
    auto dl = (left->bound.GetCenter() - node->bound.GetCenter()).Length2();
    auto dr = (right->bound.GetCenter() - node->bound.GetCenter()).Length2();

    // self distance
    auto ds = (left->bound.GetCenter() - right->bound.GetCenter()).Length2();

    auto max = std::max(std::max(dl, dr), ds);
    
    if (max == dl)
    {
        AddNode(left, node);
        return;
    }

    if (max == dr)
    {
        AddNode(left, node);
        return;
    }

    JOINT_2_NODES(root, node);
}

#undef JOINT_2_NODES

void DBVTQueryTree::QueryAABoxRecursive(Node* node, const AABox& aabox, std::Vector<void*>& output)
{
    auto left = node->left;
    auto right = node->right;

    if (node->IsLeaf())
    {
        output.push_back(node->userPtr);
        return;
    }

    if (left->bound.IsOverlap(aabox))
    {
        QueryAABoxRecursive(left, aabox, output);
    }

    if (right->bound.IsOverlap(aabox))
    {
        QueryAABoxRecursive(right, aabox, output);
    }
}

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

void DBVTQueryTree::QueryAABox(const AABox& aabox, std::Vector<void*>& output)
{
    if (m_root && m_root->bound.IsOverlap(aabox))
    {
        QueryAABoxRecursive(m_root, aabox, output);
    }
}

void DBVTQueryTree::QuerySphere(const Sphere& sphere, std::Vector<void*>& output)
{
}

void DBVTQueryTree::QueryBox(const Box& box, std::Vector<void*>& output)
{
}

void DBVTQueryTree::Query(AABBQueryTester* tester, std::Vector<void*>& output)
{
}


NAMESPACE_END