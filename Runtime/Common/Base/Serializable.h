#pragma once

#ifdef GetClassName
#undef GetClassName
#endif // GetClassName

#include "Core/Memory/SmartPointers.h"

#include "Metadata.h"
#include "Serializer.h"

#include "../Stream/ByteStream.h"

#include "UUID/UUID.h"

#include "JSON/JSON.h"

NAMESPACE_BEGIN

#define SERIALIZABLE_CLASS(className)										\
private: friend class SerializableDB;										\
private: friend class ClassMetadata;										\
template<typename T, typename ...Args>										\
friend void mheap::CallConstructor(T* begin, size_t n, Args&&... args);		\
inline static const char* ___GetClassName() {return # className;};			\
protected: inline virtual Handle<Serializable> _MakeInstance() override		\
{																			\
	static_assert(std::is_base_of_v<Serializable, className>);				\
	return  mheap::New<className>();										\
};																			\
inline virtual Serializable* _MakeInstanceRaw() override { return new className(); };	\
inline virtual SharedPtr<Serializable> _MakeInstanceShared() override { return std::make_shared<className>(); };	\
public: inline virtual const char* GetClassName() const						\
{																			\
	static_assert(std::is_base_of_v<Serializable, className>);				\
	return  # className; 													\
};

//class 

class Serializable
{
public:
	friend class Serializer;

	const UUID m_UUID;

	Serializable() : m_UUID(UUIDGenerator::Get()->GetUUID()) {};
	virtual ~Serializable() {};

protected:
	// we have 3 type of pointers using in this project
	virtual Handle<Serializable> _MakeInstance() = 0;
	virtual Serializable* _MakeInstanceRaw() = 0;
	virtual SharedPtr<Serializable> _MakeInstanceShared() = 0;

	virtual void CloneFrom(Serializer* serializer, Serializable* another) const = 0;

	/// 
	/// for data serialization
	/// 
	virtual void SerializeToBinary(Serializer* serializer, ByteStream& stream) const = 0;
	virtual void DeserializeFromBinary(Serializer* serializer, const ByteStream& stream) = 0;

	/// 
	/// we need json in development phase, in release build, binary will be used
	/// 
	virtual void SerializeToJson(Serializer* serializer, json& j) const = 0;
	virtual void DeserializeFromJson(Serializer* serializer, const json& j) = 0;

public:
	/// 
	/// for data editor
	/// 
	/// sign will be changed with each GetMetadata() called from root, use sign to break the recursive metadata chain
	/// 
	/// eg:
	///		class MyClass : public Serializable
	///		{
	///		public:
	///			float m_var1;
	///			Mat4 m_var2;
	///			Vec3 m_var3;
	///			
	///			MyClass* m_next;
	/// 
	///			virtual Handle<ClassMetadata> GetMetadata(size_t sign)
	///			{
	///				Handle<ClassMetadata> metadata = mheap::New<ClassMetadata>(this);
	///				metadata->AddProperty(&MyClass::m_var1, "var1");
	///				metadata->AddProperty(&MyClass::m_var2, "var2");
	///				metadata->AddProperty(&MyClass::m_var3, "var3");
	///				return metadata;
	///			}
	///		}
	/// 
	/// 
	virtual Handle<ClassMetadata> GetMetadata(size_t sign) = 0;

	/// 
	/// notify whenever property changed from ClassMetadata
	/// 
	virtual void OnPropertyChanged(const UnknownAddress& var, const Variant& newValue) = 0;

	virtual const char* GetClassName() const = 0;

	inline const UUID& GetUUID() const
	{
		return m_UUID;
	}
};

inline ClassMetadata::ClassMetadata(const char* className, Serializable* instance)
	: m_className(className), m_instance(instance) {};

NAMESPACE_END