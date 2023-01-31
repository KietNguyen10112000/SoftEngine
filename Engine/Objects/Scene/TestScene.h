#pragma once

#include "Scene.h"

NAMESPACE_BEGIN

class TestScene : public Scene
{
public:
	// Inherited via Scene
	virtual void ReConstruct() override;

	virtual void Synchronize() override;

};

NAMESPACE_END