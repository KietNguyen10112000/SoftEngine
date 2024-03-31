#pragma once

#include "Camera.h"

NAMESPACE_BEGIN

class CameraTPP : public Camera
{
protected:
	Handle<GameObject> m_target;
	Mat4 m_view;

	bool m_enableTPP = true;

	void Trace(Tracer* tracer)
	{
		tracer->Trace(m_target);
	}

public:
	COMPONENT_CLASS(CameraTPP);
	CameraTPP();

	virtual void OnCameraRenderBegin() override;
	//virtual Mat4 GetView() override;

	void SetTarget(const Handle<GameObject>& object);

	void SetViewPoint(const Vec3& point, const Vec3& up = Vec3::UP);

	void SetTPPEnabled(bool enable);

	inline auto IsTPPEnabled() const
	{
		return m_enableTPP;
	}

};

NAMESPACE_END