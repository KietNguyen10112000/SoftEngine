#pragma once

#include "MainSystem/Scripting/Components/Script.h"

using namespace soft;

class TestScript : public Script
{
private:
	SCRIPT_DEFAULT_METHOD(TestScript);

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		Script::Trace(tracer);
	}

public:
	virtual void OnStart() override;
	virtual void OnUpdate(float dt) override;

};

