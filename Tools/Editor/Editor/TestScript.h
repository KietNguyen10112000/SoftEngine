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

	GameObject* m_obj = nullptr;

	Transform m_childTransform;
	size_t m_childCount = 0;

public:
	virtual void OnStart() override;
	virtual void OnUpdate(float dt) override;

protected:
	virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;

};

