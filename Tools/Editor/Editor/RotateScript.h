#pragma once
#pragma once

#include "MainSystem/Scripting/Components/Script.h"

using namespace soft;

class RotateScript : public Script
{
private:
	SCRIPT_DEFAULT_METHOD(RotateScript);

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		Script::Trace(tracer);
	}

protected:
	Vec3 m_rotationSpeed = { 0,0,0 };

public:
	virtual void OnStart() override;
	virtual void OnUpdate(float dt) override;

protected:
	virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;

};

