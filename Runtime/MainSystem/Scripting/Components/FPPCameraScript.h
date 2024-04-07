#pragma once
#include "Script.h"

NAMESPACE_BEGIN

class FPPCameraScript : public Script
{
private:
	using Base = Script;
	SCRIPT_DEFAULT_METHOD(FPPCameraScript);

protected:
	float m_rotateX = 0;
	float m_rotateY = 0;
	float m_rotateZ = 0;
	Vec3 m_position = {};

	float m_speed = 10;
	float m_rotationSensi = 0.25f;

	bool m_enableFPP = true;

protected:
	TRACEABLE_FRIEND();
	inline void Trace(Tracer* tracer)
	{
		Base::Trace(tracer);
	}

protected:
	virtual void OnStart() override;

	virtual void OnUpdate(float dt) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

public:
	inline void SetFPPScriptEnable(bool enable)
	{
		m_enableFPP = enable;
	}

	inline auto IsFPPScriptEnabled() const
	{
		return m_enableFPP;
	}

	void FPPResetTransform(const Mat4& transform);

};

NAMESPACE_END