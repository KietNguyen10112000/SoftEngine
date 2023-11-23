#pragma once

#ifdef GetClassName
#undef GetClassName
#endif // GetClassName

#include "Metadata.h"
#include "Serializer.h"

#include "../Stream/ByteStream.h"

NAMESPACE_BEGIN

#define SERIALIZABLE_CLASS(className)										\
private: friend class SerializableDB;										\
private: friend class ClassMetadata;										\
inline static const char* ___GetClassName() {return # className;};			\
public: inline virtual const char* GetClassName() override					\
{																			\
	static_assert(std::is_base_of_v<Serializable, className>);				\
	return  # className; 													\
};

//class 

struct API UUIDCounter
{
	static ID GetUUID();
};

class Serializable
{
public:
	const ID m_UUID;

	Serializable() : m_UUID(UUIDCounter::GetUUID()) {};
	virtual ~Serializable() {};

public:
	/// 
	/// for data serialization
	/// 
	virtual void Serialize(Serializer* serializer) = 0;
	virtual void Deserialize(Serializer* serializer) = 0;

	///
	/// do something like destructor to start deserialize from byte stream source
	/// eg:
	///		GameObject obj = <...>
	/// 
	///		// store obj
	///		obj->Serialize(stream);
	/// 
	///		// clean up
	///		obj->CleanUp();
	/// 
	///		// reload obj from memory source
	///		obj->Deserialize(readStream);
	///		
	/// 
	virtual void CleanUp() = 0;

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

	virtual const char* GetClassName() = 0;

	inline virtual Handle<Serializable> Clone(Serializer* serializer) { return nullptr; };

	inline const ID GetUUID() const
	{
		return m_UUID;
	}
};

inline ClassMetadata::ClassMetadata(const char* className, Serializable* instance)
	: m_className(className), m_instance(instance) {};

NAMESPACE_END