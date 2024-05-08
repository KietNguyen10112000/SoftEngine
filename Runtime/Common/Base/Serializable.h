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

#define SERIALIZABLE_MEM_MANAGED			0				 // for auto memory managed class instance
#define SERIALIZABLE_MEM_SHARED				1				 // for shared ptr class instance
#define SERIALIZABLE_MEM_RAW				2				 // for raw class instance
#define SERIALIZABLE_MEM_RESOURCE			3				 // for resource

//#define _SERIALIZABLE_CLASS_IMPL_(className, memManagedImpl, memSharedImpl, memRawImpl, memType)	\
//private: friend class SerializableDB;										\
//private: friend class ClassMetadata;										\
//template<typename T, typename ...Args>										\
//friend void mheap::CallConstructor(T* begin, size_t n, Args&&... args);		\
//inline constexpr static const char* ___GetClassName() {return # className;};			\
//inline constexpr static size_t ___GetMemoryType() {return memType;};					\
//protected: inline virtual Handle<Serializable> _MakeInstance() override		\
//{																			\
//	static_assert(std::is_base_of_v<Serializable, className>);				\
//	return memManagedImpl;													\
//};																			\
//inline virtual Serializable* _MakeInstanceRaw() override { return memRawImpl; };	\
//inline virtual SharedPtr<Serializable> _MakeInstanceShared() override { return memSharedImpl; };	\
//inline virtual size_t GetMemoryType() const override {return memType;};		\
//public: inline virtual const char* GetClassName() const						\
//{																			\
//	static_assert(std::is_base_of_v<Serializable, className>);				\
//	return  # className; 													\
//};

#define _SERIALIZABLE_CLASS_IMPL_(className, memManagedImpl, memSharedImpl, memRawImpl, memType)	\
private: friend class SerializableDB;										\
private: friend class ClassMetadata;										\
template<typename T, typename ...Args>										\
friend void mheap::CallConstructor(T* begin, size_t n, Args&&... args);		\
inline constexpr static const char* ___GetClassName() {return # className;};		\
inline constexpr static size_t ___GetMemoryType() {return size_t(memType);};		\
inline virtual size_t GetMemoryType() const override {return size_t(memType);};		\
public: inline virtual const char* GetClassName() const						\
{																			\
	static_assert(std::is_base_of_v<Serializable, className>);				\
	return  # className; 													\
};

#define _SERIALIZABLE_CLASS_IMPL_0(className) _SERIALIZABLE_CLASS_IMPL_(\
	className,									\
	mheap::New<className>(),					\
	nullptr,nullptr,							\
	SERIALIZABLE_MEM_MANAGED					\
)

#define _SERIALIZABLE_CLASS_IMPL_1(className) _SERIALIZABLE_CLASS_IMPL_(\
	className,									\
	nullptr, std::make_shared<className>(),		\
	nullptr,									\
	SERIALIZABLE_MEM_SHARED						\
)

#define _SERIALIZABLE_CLASS_IMPL_2(className) _SERIALIZABLE_CLASS_IMPL_(\
	className,									\
	nullptr, nullptr, new className(),			\
	SERIALIZABLE_MEM_RAW						\
)

#define _SERIALIZABLE_CLASS_0(className)					_SERIALIZABLE_CLASS_IMPL_0(className)
#define _SERIALIZABLE_CLASS_1(className, memConstraint)		_SERIALIZABLE_CLASS_IMPL_##memConstraint(className)

#define _SERIALIZABLE_GET_3RD_ARG(arg1, arg2, arg3, ...) arg3
#define _SERIALIZABLE_MACRO_CHOOSER(...) \
    _SERIALIZABLE_GET_3RD_ARG(0, __VA_ARGS__, _SERIALIZABLE_CLASS_1, _SERIALIZABLE_CLASS_0, )

#define SERIALIZABLE_CLASS(className, ...) _SERIALIZABLE_MACRO_CHOOSER(__VA_ARGS__)(className, __VA_ARGS__)

//class 

class Serializable
{
public:
	friend class Serializer;

	const UUID m_UUID;

	Serializable() : m_UUID(UUIDGenerator::Get()->GetUUID()) {};
	virtual ~Serializable() {};

protected:
	//// we have 3 type of pointers using in this project
	//virtual Handle<Serializable> _MakeInstance() = 0;
	//virtual Serializable* _MakeInstanceRaw() = 0;
	//virtual SharedPtr<Serializable> _MakeInstanceShared() = 0;

	virtual void CloneFrom(Serializer* serializer, Serializable* another) = 0;

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
	virtual size_t GetMemoryType() const = 0;

	inline const UUID& GetUUID() const
	{
		return m_UUID;
	}
};

inline ClassMetadata::ClassMetadata(const char* className, Serializable* instance)
	: m_className(className), m_instance(instance) {};

NAMESPACE_END