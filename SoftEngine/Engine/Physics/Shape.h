#pragma once

#include <vector>

#include "Math/Shape.h"
#include "Math/Transform.h"


/// 
/// This class is used for PhysicsObject construction
/// 
class Shape
{
public:
	enum TYPE
	{
		UNKNOWN,
		SPHERE,
		CAPSULE,
		BOX
	};

	union
	{
		SphereShape shpere;
		CapsuleShape capsule;
		BoxShape box = { 0,0,0 };
	} m_ = {};

	TYPE m_type = UNKNOWN;

private:
	//public:
	friend class CompoundShape;
	friend class std::vector<std::pair<CompoundShape, Transform>>;

	inline Shape() {};

public:
	inline auto Type() const { return m_type; };

	template <typename T>
	inline T& As() const 
	{ 
#ifdef _DEBUG
		assert(TypeOf<T>() == m_type);
#endif // DEBUG
		return *(T*)(&m_); 
	};

public:
	template <typename T>
	static TYPE TypeOf()
	{
		if constexpr (std::is_same_v<T, SphereShape>) return SPHERE;
		if constexpr (std::is_same_v<T, CapsuleShape>) return CAPSULE;
		if constexpr (std::is_same_v<T, BoxShape>) return BOX;
		return UNKNOWN;
	};

	template <typename T, typename... Args>
	static Shape Get(Args&&... args)
	{
		Shape ret;
		ret.m_type = TypeOf<T>();
		*reinterpret_cast<T*>(&ret.m_.box) = T(std::forward<Args>(args)...);
		return ret;
	};
	
};



/// 
/// This class is used for PhysicsObject construction
/// 
class CompoundShape
{
public:
	Shape m_self;
	std::vector<std::pair<CompoundShape, Transform>> m_childs;

public:
	inline CompoundShape(const Shape& shape) : m_self(shape) {};

public:
	inline void AddChild(const CompoundShape& shape, const Transform& transform)
	{ 
		m_childs.push_back({ shape, transform });
	};
	// no remove child =)))

	inline const CompoundShape& GetChild(size_t i) const { return m_childs[i].first; };
	inline size_t ChildsCount() const { return m_childs.size(); };

};