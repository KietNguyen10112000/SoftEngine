#pragma once

#include <vector>

template <typename T>
class NodeDefaultDataDeleter
{
public:
	inline static void Delete(T& data)
	{
		if constexpr (std::is_pointer_v<T>)
			delete data;
	};

};

template <typename _Data, typename _DataDeleter = NodeDefaultDataDeleter<_Data>>
class Node
{
public:
	_Data m_data;

	Node* m_parent = 0;
	std::vector<Node> m_childs;

public:
	inline ~Node()
	{
		_DataDeleter::Delete(m_data);
	};

public:
	template <typename _Func1, typename _Func2, typename ..._Args>
	inline void Traverse(_Func1 prevCall, _Func2 postCall, _Args&&... args)
	{
		if constexpr (!(std::is_same_v<std::decay_t<_Func1>, std::nullptr_t>)) 
			prevCall(this, std::forward<_Args>(args) ...);

		for (auto& child : m_childs)
		{
			child.Traverse(prevCall, postCall, args);
		}

		if constexpr (!(std::is_same_v<std::decay_t<_Func2>, std::nullptr_t>))
			postCall(this, std::forward<_Args>(args) ...);
	}

	inline auto& Data() noexcept { return m_data; };

};