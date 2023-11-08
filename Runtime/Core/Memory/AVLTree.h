#include <iostream>

#include <vector>

#include <string>

namespace avl 
{

template <typename Node>
class Tree
{
public:
#define DefineHasClassMethod(method) 										\
	template <typename T>													\
	class Has_##method														\
	{																		\
		typedef char one;													\
		struct two { char x[2]; };											\
		template <typename C> static one test( decltype(&C::method) ) ;		\
		template <typename C> static two test(...); 						\
	public:																	\
		enum { value = sizeof(test<T>(0)) == sizeof(char) };				\
	};

	DefineHasClassMethod(BF);
	DefineHasClassMethod(Parent);
	DefineHasClassMethod(Left);
	DefineHasClassMethod(Right);	
	DefineHasClassMethod(Key);	
	DefineHasClassMethod(Print);

#undef DefineHasClassMethod
	
	static_assert(Has_BF<Node>::value, "Node must provides accessor method \"Node*& BF();\"");
	static_assert(Has_Parent<Node>::value, "Node must provides accessor method \"Node*& Parent();\"");
	static_assert(Has_Left<Node>::value, "Node must provides accessor method \"Node*& Left();\"");
	static_assert(Has_Right<Node>::value, "Node must provides accessor method \"Node*& Right();\"");
	static_assert(Has_Key<Node>::value, "Node must provides accessor method \"KeyType& Key();\"");
	static_assert(Has_Print<Node>::value, "Node must provides method \"void Print();\"");
	
	using BFType = std::remove_reference_t<decltype(std::declval<Node>().BF())>;
	using KeyType = std::remove_reference_t<decltype(std::declval<Node>().Key())>;
	
public:
	Node* m_root = 0;
	
//public:
//	std::vector<const wchar_t*> m_printStack;
	
protected:
	using node = Node;
	
#define _left_child(x) x->Left()
#define _right_child(x) x->Right()
#define parent(x) x->Parent()
#define BF(x) x->BF()
#define null nullptr
	
	/*
	Input:	X = root of subtree to be rotated left
			Z = right child of X, Z is right-heavy
			    with height == Height(LeftSubtree(X))+2
	Result:	new root of rebalanced subtree
	*/
	node* rotate_Left(node* X, node* Z) 
	{
#define left_child _left_child
#define right_child _right_child
		//std::cout << "Rotate Left\n";
	    // Z is by 2 higher than its sibling
	    auto t23 = left_child(Z); // Inner child of Z
	    right_child(X) = t23;
	    if (t23 != null)
	        parent(t23) = X;
	    left_child(Z) = X;
	    parent(X) = Z;
	    // 1st case, BF(Z) == 0,
	    //   only happens with deletion, not insertion:
	    if (BF(Z) == 0) 
	    { // t23 has been of same height as t4
	        BF(X) = +1;   // t23 now higher
	        BF(Z) = -1;   // t4 now lower than X
	    } 
	    else
	    { // 2nd case happens with insertion or deletion:
	        BF(X) = 0;
	        BF(Z) = 0;
	    }
	    return Z; // return new root of rotated subtree
	    
#undef left_child
#undef right_child 
	}
	
	node* _rotate_Right(node* X, node* Z) 
	{
#define left_child _right_child
#define right_child _left_child
		//std::cout << "Rotate Right\n";
	    // Z is by 2 higher than its sibling
	    auto t23 = left_child(Z); // Inner child of Z
	    right_child(X) = t23;
	    if (t23 != null)
	        parent(t23) = X;
	    left_child(Z) = X;
	    parent(X) = Z;
	    // 1st case, BF(Z) == 0,
	    //   only happens with deletion, not insertion:
	    if (BF(Z) == 0) 
	    { // t23 has been of same height as t4
	        BF(X) = -1;   // t23 now higher
	        BF(Z) = +1;   // t4 now lower than X
	    } 
	    else
	    { // 2nd case happens with insertion or deletion:
	        BF(X) = 0;
	        BF(Z) = 0;
	    }
	    return Z; // return new root of rotated subtree
	    
#undef left_child
#undef right_child 
	}
	
#define rotate_Right(x, z) _rotate_Right(x, z)
	
	/*
	Input:	X = root of subtree to be rotated
			Z = its right child, left-heavy
			    with height == Height(LeftSubtree(X))+2
	Result:	new root of rebalanced subtree
	*/
	node* rotate_RightLeft(node* X, node* Z)
	{
#define left_child _left_child
#define right_child _right_child
		//std::cout << "Rotate RightLeft\n";
	    // Z is by 2 higher than its sibling
	    auto Y = left_child(Z); // Inner child of Z
	    // Y is by 1 higher than sibling
	    auto t3 = right_child(Y);
	    left_child(Z) = t3;
	    if (t3 != null)
	        parent(t3) = Z;
	    right_child(Y) = Z;
	    parent(Z) = Y;
	    auto t2 = left_child(Y);
	    right_child(X) = t2;
	    if (t2 != null)
	        parent(t2) = X;
	    left_child(Y) = X;
	    parent(X) = Y;
	    // 1st case, BF(Y) == 0,
	    //   only happens with deletion, not insertion:
	    if (BF(Y) == 0) 
	    {
	        BF(X) = 0;
	        BF(Z) = 0;
	    } 
	    else
	    // other cases happen with insertion or deletion:
	        if (BF(Y) > 0) 
	    	{ // t3 was higher
	            BF(X) = -1;  // t1 now higher
	            BF(Z) = 0;
	        } 
	        else 
	        {
	            // t2 was higher
	            BF(X) = 0;
	            BF(Z) = +1;  // t4 now higher
	        }
	        
	    BF(Y) = 0;
	    return Y; // return new root of rotated subtree
	        
#undef left_child
#undef right_child 
	}

	node* _rotate_LeftRight(node* X, node* Z)
	{
#define left_child _right_child
#define right_child _left_child
		//std::cout << "Rotate LeftRight\n";		
	    // Z is by 2 higher than its sibling
	    auto Y = left_child(Z); // Inner child of Z
	    // Y is by 1 higher than sibling
	    auto t3 = right_child(Y);
	    left_child(Z) = t3;
	    if (t3 != null)
	        parent(t3) = Z;
	    right_child(Y) = Z;
	    parent(Z) = Y;
	    auto t2 = left_child(Y);
	    right_child(X) = t2;
	    if (t2 != null)
	        parent(t2) = X;
	    left_child(Y) = X;
	    parent(X) = Y;
	    // 1st case, BF(Y) == 0,
	    //   only happens with deletion, not insertion:
	    if (BF(Y) == 0) 
	    {
	        BF(X) = 0;
	        BF(Z) = 0;
	    } 
	    else
	    // other cases happen with insertion or deletion:
	        if (BF(Y) < 0) 
	    	{ // t3 was higher
	            BF(X) = +1;  // t1 now higher
	            BF(Z) = 0;
	        } 
	        else 
	        {
	            // t2 was higher
	            BF(X) = 0;
	            BF(Z) = -1;  // t4 now higher
	        }
	        
	    BF(Y) = 0;
	    return Y; // return new root of rotated subtree
	        
#undef left_child
#undef right_child 
	}
	
#define rotate_LeftRight(x, z) _rotate_LeftRight(x, z)
	
#define left_child _left_child
#define right_child _right_child

	void insert_Rebalance(node*& root, node* Z)
	{
		node* X = null;
		node* G = null;
		node* N = null;
		for (X = parent(Z); X != null; X = parent(Z)) { // Loop (possibly up to the root)
			// BF(X) has to be updated:
		    if (Z == right_child(X)) { // The right subtree increases
		        if (BF(X) > 0) { // X is right-heavy
		            // ==> the temporary BF(X) == +2
		            // ==> rebalancing is required.
		            G = parent(X); // Save parent of X around rotations
		            if (BF(Z) < 0)                  // Right Left Case  (see figure 3)
		                N = rotate_RightLeft(X, Z); // Double rotation: Right(Z) then Left(X)
		            else                            // Right Right Case (see figure 2)
		                N = rotate_Left(X, Z);      // Single rotation Left(X)
		            // After rotation adapt parent link
		        } else {
		            if (BF(X) < 0) {
		                BF(X) = 0; // Z’s height increase is absorbed at X.
		                break; // Leave the loop
		            }
		            BF(X) = +1;
		            Z = X; // Height(Z) increases by 1
		            continue;
		        }
		    } else { // Z == left_child(X): the left subtree increases
		        if (BF(X) < 0) { // X is left-heavy
		            // ==> the temporary BF(X) == -2
		            // ==> rebalancing is required.
		            G = parent(X); // Save parent of X around rotations
		            if (BF(Z) > 0)                  // Left Right Case
		                N = rotate_LeftRight(X, Z); // Double rotation: Left(Z) then Right(X)
		            else                            // Left Left Case
		                N = rotate_Right(X, Z);     // Single rotation Right(X)
		            // After rotation adapt parent link
		        } else {
		            if (BF(X) > 0) {
		                BF(X) = 0; // Z’s height increase is absorbed at X.
		                break; // Leave the loop
		            }
		            BF(X) = -1;
		            Z = X; // Height(Z) increases by 1
		            continue;
		        }
		    }
		    // After a rotation adapt parent link:
		    // N is the new root of the rotated subtree
		    // Height does not change: Height(N) == old Height(X)
		    parent(N) = G;
		    if (G != null) {
		        if (X == left_child(G))
		            left_child(G) = N;
		        else
		            right_child(G) = N;
		    } else
		        //tree->root = N; // N is the new root of the total tree
		        root = N;
		    break;
		    // There is no fall thru, only break; or continue;
		}
		// Unless loop is left via break, the height of the total tree increases by 1.
	}
	
	void remove_Rebalance(node*& root, node* N)
	{
		node* G = null;
		node* X = null;
		node* Z = null;
		BFType b;
		
		for (X = parent(N); X != null; X = G) { // Loop (possibly up to the root)
		    G = parent(X); // Save parent of X around rotations
		    // BF(X) has not yet been updated!
		    if (N == left_child(X)) { // the left subtree decreases
		        if (BF(X) > 0) { // X is right-heavy
		            // ==> the temporary BF(X) == +2
		            // ==> rebalancing is required.
		            Z = right_child(X); // Sibling of N (higher by 2)
		            b = BF(Z);
		            if (b < 0)                      // Right Left Case  (see figure 3)
		                N = rotate_RightLeft(X, Z); // Double rotation: Right(Z) then Left(X)
		            else                            // Right Right Case (see figure 2)
		                N = rotate_Left(X, Z);      // Single rotation Left(X)
		            // After rotation adapt parent link
		        } else {
		            if (BF(X) == 0) {
		                BF(X) = +1; // N’s height decrease is absorbed at X.
		                break; // Leave the loop
		            }
		            N = X;
		            BF(N) = 0; // Height(N) decreases by 1
		            continue;
		        }
		    } else { // (N == right_child(X)): The right subtree decreases
		        if (BF(X) < 0) { // X is left-heavy
		            // ==> the temporary BF(X) == -2
		            // ==> rebalancing is required.
		            Z = left_child(X); // Sibling of N (higher by 2)
		            b = BF(Z);
		            if (b > 0)                      // Left Right Case
		                N = rotate_LeftRight(X, Z); // Double rotation: Left(Z) then Right(X)
		            else                            // Left Left Case
		                N = rotate_Right(X, Z);     // Single rotation Right(X)
		            // After rotation adapt parent link
		        } else {
		            if (BF(X) == 0) {
		                BF(X) = -1; // N’s height decrease is absorbed at X.
		                break; // Leave the loop
		            }
		            N = X;
		            BF(N) = 0; // Height(N) decreases by 1
		            continue;
		        }
		    }
		    // After a rotation adapt parent link:
		    // N is the new root of the rotated subtree
		    parent(N) = G;
		    if (G != null) {
		        if (X == left_child(G))
		            left_child(G) = N;
		        else
		            right_child(G) = N;
		    } else
		        //tree->root = N; // N is the new root of the total tree
		        root = N;
		 
		    if (b == 0)
		        break; // Height does not change: Leave the loop
		 
		    // Height(N) decreases by 1 (== old Height(X)-1)
		}
		// If (b != 0) the height of the total tree decreases by 1.
	}
	
#undef _left_child
#undef _right_child
#undef left_child
#undef right_child
	
#undef parent
#undef BF
#undef null
	
	Node* find(KeyType key)
	{
		auto cur = m_root;
		while (cur)
		{
			if (key < cur->Key())
			{
				cur = cur->Left();
				continue;
			}
			
			if (key > cur->Key())
			{
				cur = cur->Right();
				continue;
			}
			
			break;
		}
		return cur;
	}
	
	Node* find_Insert(KeyType key)
	{
		auto cur = m_root;
		Node* t = 0;
		
		while (true)
		{
			if (key < cur->Key() && (t = cur->Left()))
			{
				cur = t;
				continue;
			}
			
			if (key > cur->Key() && (t = cur->Right()))
			{
				cur = t;
				continue;
			}
			
			break;
		}
		return cur;
	}
	
	inline Node* insert(Node*& root, node* N)
	{
		if (root == 0) 
		{
			root = N;
			return N;
		}
		
		auto key = N->Key();
		
		auto cur = find_Insert(key);
		
		if (cur && key == cur->Key())
		{
			return cur;
		}
		
		N->Parent() = cur;
		
		if (key < cur->Key())
		{
			cur->Left() = N;
		}
		else
		{
			cur->Right() = N;
		}		

		insert_Rebalance(root, N);
		
		return N;
	}
	
	inline void ShiftNode(Node* u, Node* v)
	{
		if (u->Parent() == nullptr)	
			m_root = v;
		else if (u == u->Parent()->Left())
		{
			u->Parent()->Left() = v;
		}
		else
		{
			u->Parent()->Right() = v;
		}

		if (v != nullptr)
			v->Parent() = u->Parent();
	};
	
	inline Node* BSTMinimum(Node* x)
	{
	   while (x->Left() != nullptr)
	     x = x->Left();
	   return x;
	}
	
	inline Node* BSTSuccessor(Node* x)
	{
	   if (x->Right() != nullptr)
	     return BSTMinimum(x->Right());
	   
	   auto y = x->Parent();
	   while (y != nullptr && x == y->Right())
	   {
	     x = y;
	     y = y->Parent();
	   }
	   
	   return y;
	}
	
	// fakeZ == -1 => left
	// fakeZ == 1 => right
	Node* BSTDelete(Node* z)
	{
		if (z->Left() == nullptr && z->Right() == nullptr)
		{	
			if (z == m_root)
			{
				m_root = 0;
				return 0;
			}
			return z;
		}
		
		if (z->Left() == nullptr)
		{
			ShiftNode(z, z->Right());
			return z->Right();
		}
		else if (z->Right() == nullptr)
		{
			ShiftNode(z, z->Left());
			return z->Left();
		}
		else
		{
			auto y = BSTSuccessor(z);
			
			Node* x = 0;
			if (y->Parent() != z)
			{
				ShiftNode(y, y->Right());
				y->Right() = z->Right();
				y->Right()->Parent() = y;			
				
				x = y->Parent();
			}
			ShiftNode(z, y);
			y->Left() = z->Left();
			y->Left()->Parent() = y;
			
			y->BF() = z->BF();
			
			if (x == nullptr)
			{
				if (y->Right()) return y->Right();
				
				z->Parent() = y;
				y->Right() = z;
				//*fakeZ = 1;
				return z;
			}
			
			//z->right->bf += 1;
					
			if (x->Left()) return x->Left();
			
			z->Parent() = x;
			x->Left() = z;
			//*fakeZ = -1;
			
			return z;
		}
	};
	
	int Depth(Node* node)
	{
		if (node == 0) return 0;
		
		return std::max(Depth(node->Left()), Depth(node->Right())) + 1;
	};
	
	void _CheckAVL(Node* node)
	{
		if (node == 0) return;
		
		auto lD = Depth(node->Left());
		auto rD = Depth(node->Right());
		
		if (rD - lD != node->BF()) 
		{
			/*std::cout << "[CheckAVL]: " << node->Print() << "\n";
			std::cout << "\n\n================================================\n\n";
			Print();*/
			exit(-1);
		}
		
		_CheckAVL(node->Left());
		_CheckAVL(node->Right());
	}
	
	inline void erase(node* N)
	{
		auto z = BSTDelete(N);

		if (z)
		{
			remove_Rebalance(m_root, z);
		}
		else
		{
			return;
		}
			
		if (!N->Parent()) return;
			
		if (N->Parent()->Right() == N)
		{
			N->Parent()->Right() = 0;
		}
		else if (N->Parent()->Left() == N)
		{
			N->Parent()->Left() = 0;
		}
	};
//	
//private:
//	void printBT(const std::wstring& prefix, Node* node, bool isLeft)
//	{		
//		if (node == nullptr) return;
//		
//		for (auto& v : m_printStack)
//		{
//			std::wcout << v;
//		}
//	
//		if (!prefix.empty())
//		{
//			std::wcout << L"│\n" << prefix;
//			if (node->Parent() && node->Parent()->Right() == nullptr) 
//				std::wcout << L"└──[L]: ";
//	    	else
//	    		std::wcout << (isLeft ? L"├──[L]: " : L"└──[R]: " );
//	    }
//		else
//			std::cout << " [ROOT]: ";
//	    	
//	    if( node != nullptr )
//	    {
//	    	node->Print();
//	        // enter the next tree level - left and right branch
//	    	m_printStack.push_back(isLeft ? L"│   " : L"     ");
//	        printBT( prefix + (isLeft ? L"│   " : L"     "), node->Left(), true);
//	    	m_printStack.pop_back();
//	    	
//	    	m_printStack.push_back(isLeft ? L"│   " : L"     ");
//	        printBT( prefix + (isLeft ? L"│   " : L"     "), node->Right(), false);
//	    	m_printStack.pop_back();
//	    }
//	    else
//	    {
//	  		std::cout << "NULL\n";
//	  	}
//	}
	
public:
	inline Node* Insert(Node* node)
	{
#ifdef _STRESS_DEBUG
		auto ret = insert(m_root, node);
		CheckAVL();
		return ret;
#else
		return insert(m_root, node);
#endif // _DEBUG
	};
	
	inline void Erase(Node* node)
	{
#ifdef _STRESS_DEBUG
		erase(node);
		CheckAVL();
#else
		erase(node);
#endif // _DEBUG
	};
	
	inline Node* Find(KeyType key)
	{
		return find(key);
	};
	
	/*inline void Print()
	{
	    printBT(L"", m_root, false);    
	}*/
	
	inline void CheckAVL()
	{
		_CheckAVL(m_root);
	}
	
};

}