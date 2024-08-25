#pragma once

#include "RigidBodyInspector.h"

namespace soft
{
	class CharacterController;
}

class CCTInspector : public RigidBodyInspector
{
public:
	CCTInspector(CharacterController* cct, ClassMetadata* metadata);

	virtual void Inspect() override;

};

