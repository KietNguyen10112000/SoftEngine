#pragma once

#include "PhysicsMaterial.h"

NAMESPACE_BEGIN

class PhysicsDefaultMaterial : public Singleton<PhysicsDefaultMaterial>
{
private:
	SharedPtr<PhysicsMaterial> m_material;

public:
	PhysicsDefaultMaterial();

public:
	inline const auto& GetDefault() const
	{
		return m_material;
	}

};

NAMESPACE_END