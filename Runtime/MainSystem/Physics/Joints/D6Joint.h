#pragma once

#include "Joint.h"

NAMESPACE_BEGIN

class API D6Joint : public Joint
{
	SERIALIZABLE_CLASS(D6Joint);
public:
	struct MOTION_AXIS
	{
		enum ENUM
		{
			X,
			Y,
			Z,

			TWIST_X,
			SWING_Y,
			SWING_Z,
		};
	};

	struct MOTION_TYPE
	{
		enum ENUM
		{
			LOCKED,
			LIMITED,
			FREE
		};
	};

	struct DRIVE_TYPE
	{
		enum ENUM
		{
			X,
			Y,
			Z,

			TWIST,
			SWING,
			SLERP
		};
	};

	struct SwingLimit : public BaseLimit
	{
		float yLimitAngle = PI / 2.0f;
		float zLimitAngle = PI / 2.0f;
	};

	struct TwistLimit : public BaseLimit
	{
		float lowerLimit = -PI / 2.0f;
		float upperLimit = PI / 2.0f;
	};

	struct DistanceLimit : public BaseLimit
	{
		float distance = FLT_MAX;
	};

	struct LinearLimit : public BaseLimit
	{
		float lower = -FLT_MAX / 3.0f;
		float upper = FLT_MAX / 3.0f;
	};

	struct DriveLimit
	{
		float stiffness = 0.0f;
		float damping = 0.0f;
		float forceLimit = FLT_MAX;
		bool isAcceleration = false;
	};

public:
	inline D6Joint() {};
	D6Joint(
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
	void SetMotion(const MOTION_AXIS::ENUM& axis, const MOTION_TYPE::ENUM& type);
	MOTION_TYPE::ENUM GetMotion(const MOTION_AXIS::ENUM& axis) const;

	void SetSwingLimit(const SwingLimit& limit);
	SwingLimit GetSwingLimit() const;

	void SetTwistLimit(const TwistLimit& limit);
	TwistLimit GetTwistLimit() const;

	void SetDistanceLimit(const DistanceLimit& limit);
	DistanceLimit GetDistanceLimit() const;

	void SetLinearLimit(const MOTION_AXIS::ENUM& axis, const LinearLimit& limit);
	LinearLimit GetLinearLimit(const MOTION_AXIS::ENUM& axis) const;

	void SetDrive(const DRIVE_TYPE::ENUM& type, const DriveLimit& limit);
	DriveLimit GetDrive(const DRIVE_TYPE::ENUM& type) const;
	void SetDrivePosition(const Transform& localOfBody0);
	Transform GetDrivePosition() const;
	void SetDriveVelocity(const Vec3& linear, const Vec3& angular);
	void GetDriveVelocity(Vec3& linear, Vec3& angular) const;
};

NAMESPACE_END