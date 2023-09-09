#pragma once

#include "RenderingComponent.h"

NAMESPACE_BEGIN

class Camera : public RenderingComponent
{
private:
	friend class RenderingSystem;

	ID m_activeID = INVALID_ID;

	Mat4 m_proj;
	Mat4 m_view;

public:
	// Inherited via RenderingComponent
	virtual void Serialize(ByteStream& stream) override;

	virtual void Deserialize(ByteStreamRead& stream) override;

	virtual void CleanUp() override;

	virtual Handle<ClassMetadata> GetMetadata(size_t sign) override;

	virtual void OnPropertyChanged(const UnknownAddress& var) override;

	virtual void OnComponentAdded() override;

	virtual void OnComponentRemoved() override;

	virtual void OnTransformChanged() override;

	virtual AABox GetGlobalAABB() override;

	virtual void Render() override;

};

NAMESPACE_END