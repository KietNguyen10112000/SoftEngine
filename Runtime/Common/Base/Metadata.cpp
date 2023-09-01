#include "Metadata.h"

#include "Serializable.h"

#include "MetadataUtils.h"

#include <charconv>

NAMESPACE_BEGIN

#ifdef _DEBUG_METADATA
ID MetadataClassCounter::s_count = 0;
#endif

void Accessor::Set(const String& input)
{
	m_setter(input, m_var, m_instance);

	if (m_instance)
		m_instance->OnPropertyChanged(m_var);
}

Accessor Accessor::For(const char* name, Vec3& vec, Serializable* instance)
{
	static auto format = "[%number:<x> %number:<y> %number:<z>]";
	return Accessor(
		name,
		format,
		&vec,
		[](const String& input, UnknownAddress& var, Serializable* instance) -> void
		{
			auto& vec = var.As<Vec3>();
			auto& values = MetadataParser::Get()->GetValueChain(format, input);
			if (values.size() == 3)
			{
				std::from_chars(values[0].data(), values[0].data() + values[0].size(), vec.x);
				std::from_chars(values[1].data(), values[1].data() + values[1].size(), vec.y);
				std::from_chars(values[2].data(), values[2].data() + values[2].size(), vec.z);
			}
		},
		[](UnknownAddress& var, Serializable* instance) -> String
		{
			auto& vec = var.As<Vec3>();
			return String::Format("[{} {} {}]", vec.x, vec.y, vec.z);
		},
		instance
	);
}

Accessor Accessor::For(const char* name, Quaternion& quad, Serializable* instance)
{
	static auto format = "[%number:<x> %number:<y> %number:<z> %number:<w>]";
	return Accessor(
		name,
		format,
		&quad,
		[](const String& input, UnknownAddress& var, Serializable* instance) -> void
		{
			auto& quad = var.As<Quaternion>();
			auto& values = MetadataParser::Get()->GetValueChain(format, input);
			if (values.size() == 4)
			{
				std::from_chars(values[0].data(), values[0].data() + values[0].size(), quad.x);
				std::from_chars(values[1].data(), values[1].data() + values[1].size(), quad.y);
				std::from_chars(values[2].data(), values[2].data() + values[2].size(), quad.z);
				std::from_chars(values[3].data(), values[3].data() + values[3].size(), quad.w);
			}
		},
		[](UnknownAddress& var, Serializable* instance) -> String
		{
			auto& quad = var.As<Quaternion>();
			return String::Format("[{} {} {} {}]", quad.x, quad.y, quad.z, quad.w);
		},
		instance
	);
}

Accessor Accessor::For(const char* name, Mat4& mat, Serializable* instance)
{
	static auto format = 
		"[%number %number %number %number]:<row 1> \n"
		"[%number %number %number %number]:<row 2> \n"
		"[%number %number %number %number]:<row 3> \n"
		"[%number %number %number %number]:<row 4>";
	return Accessor(
		name,
		format,
		&mat,
		[](const String& input, UnknownAddress& var, Serializable* instance) -> void
		{
			auto& mat = var.As<Mat4>();
			auto& values = MetadataParser::Get()->GetValueChain(format, input);
			if (values.size() == 16)
			{
				auto* data = &mat[0][0];
				for (size_t i = 0; i < 16; i++)
				{
					std::from_chars(values[i].data(), values[i].data() + values[i].size(), data[i]);
				}
			}
		},
		[](UnknownAddress& var, Serializable* instance) -> String
		{
			auto& mat = var.As<Mat4>();
			return String::Format(
				"[{} {} {} {}] \n"
				"[{} {} {} {}] \n"
				"[{} {} {} {}] \n"
				"[{} {} {} {}]",
				mat[0][0], mat[0][1], mat[0][2], mat[0][3],
				mat[1][0], mat[1][1], mat[1][2], mat[1][3],
				mat[2][0], mat[2][1], mat[2][2], mat[2][3],
				mat[3][0], mat[3][1], mat[3][2], mat[3][3]
			);
		},
		instance
	);
}

Accessor Accessor::For(const char* name, Transform& transform, Serializable* instance)
{
	static auto format =
		"[%number:<x> %number:<y> %number:<z>]:<scale>		\n"
		"[%number:<x> %number:<y> %number:<z>]:<rotation>	\n"
		"[%number:<x> %number:<y> %number:<z>]:<position>";
	return Accessor(
		name,
		format,
		&transform,
		[](const String& input, UnknownAddress& var, Serializable* instance) -> void
		{
			auto& transform = var.As<Transform>();
			auto& values = MetadataParser::Get()->GetValueChain(format, input);
			if (values.size() == 9)
			{
				auto& scale = transform.Scale();
				std::from_chars(values[0].data(), values[0].data() + values[0].size(), scale.x);
				std::from_chars(values[1].data(), values[1].data() + values[1].size(), scale.y);
				std::from_chars(values[2].data(), values[2].data() + values[2].size(), scale.z);

				Vec3 eulers = {};
				auto& rotation = transform.Rotation();
				std::from_chars(values[3].data(), values[3].data() + values[3].size(), eulers.x);
				std::from_chars(values[4].data(), values[4].data() + values[4].size(), eulers.y);
				std::from_chars(values[5].data(), values[5].data() + values[5].size(), eulers.z);
				rotation = Quaternion(eulers);

				auto& position = transform.Translation();
				std::from_chars(values[6].data(), values[6].data() + values[6].size(), position.x);
				std::from_chars(values[7].data(), values[7].data() + values[7].size(), position.y);
				std::from_chars(values[8].data(), values[8].data() + values[8].size(), position.z);
			}
		},
		[](UnknownAddress& var, Serializable* instance) -> String
		{
			auto& transform = var.As<Transform>();
			auto rotation = transform.Rotation().ToEulerAngles();
			return String::Format(
				"[{} {} {}] \n"
				"[{} {} {}] \n"
				"[{} {} {}]",
				transform.Scale().x, transform.Scale().y, transform.Scale().z,
				rotation.x, rotation.y, rotation.z,
				transform.Translation().x, transform.Translation().y, transform.Translation().z
			);
		},
		instance
	);
}


NAMESPACE_END