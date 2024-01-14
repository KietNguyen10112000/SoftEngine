#pragma once
#include "Script.h"

NAMESPACE_BEGIN

class CharacterController;

class TestScript : public Script
{
private:
	using Base = Script;
	SCRIPT_DEFAULT_METHOD(TestScript);

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		Base::Trace(tracer);
	}

	CharacterController* controller = nullptr;

	float m_motionY = 0;
	float m_prevPosY1 = 0;
	float m_prevPosY2 = 0;

protected:
	virtual void OnStart() override;

	virtual void OnUpdate(float dt) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

};

NAMESPACE_END