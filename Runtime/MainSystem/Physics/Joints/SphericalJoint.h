#pragma once

#include "Joint.h"

NAMESPACE_BEGIN

class API SphericalJoint : public Joint
{
	SERIALIZABLE_CLASS(SphericalJoint);
public:
	struct Limit : public Joint::BaseLimit
	{
		float yLimitAngle = PI / 2.0f;
		float zLimitAngle = PI / 2.0f;
	};

	inline SphericalJoint() {};
	SphericalJoint(
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

};

NAMESPACE_END