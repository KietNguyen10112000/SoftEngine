#pragma once

#include "Joint.h"

NAMESPACE_BEGIN

class RevoluteJoint : public Joint
{
public:
	RevoluteJoint(
		const Handle<RigidBody>& body0, 
		const Transform& localFrame0, 
		const Handle<RigidBody>& body1, 
		const Transform& localFrame1
	);

};

NAMESPACE_END