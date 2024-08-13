#pragma once

#include "Joint.h"

NAMESPACE_BEGIN

class API RevoluteJoint : public Joint
{
	SERIALIZABLE_CLASS(RevoluteJoint);
public:
	struct Limit : public Joint::BaseLimit
	{
		float upperLimit = PI / 2.0f;
		float lowerLimit = -PI / 2.0f;
	};

	inline RevoluteJoint() {};
	RevoluteJoint(
		const Handle<RigidBody>& body0, 
		const Transform& localFrame0, 
		const Handle<RigidBody>& body1, 
		const Transform& localFrame1
	);

protected:
	// Inherited via Joint
	void CloneFrom(Serializer* serializer, Serializable* another) override;

	void SerializeToBinary(Serializer* serializer, ByteStream& stream) const override;

	void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) override;

	void SerializeToJson(Serializer* serializer, json& j) const override;

	void DeserializeFromJson(Serializer* serializer, const json& j) override;

	Handle<ClassMetadata> GetMetadata(size_t sign) override;

	void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) override;

public:
	void SetLimit(const Limit& limit);
	Limit GetLimit() const;
	void SetEnableLimit(bool enable);
	bool IsEnableLimit() const;

	void SetDriveVelocity(float v);
	float GetDriveVelocity() const;
	void SetEnableDriveVelocity(bool enable);
	bool IsEnableDriveVelocity() const;

	void SetDriveForceLimit(float v);
	float GetDriveForceLimit() const;
};

NAMESPACE_END