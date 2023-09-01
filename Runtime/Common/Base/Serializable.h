#pragma once

#include "Metadata.h"

#include "../Stream/ByteStream.h"

NAMESPACE_BEGIN

class Serializable
{
public:
	virtual ~Serializable() {};

public:
	/// 
	/// for data serialization
	/// 
	virtual void Serialize(ByteStream& stream) = 0;
	virtual void Deserialize(ByteStreamRead& stream) = 0;

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
	virtual void OnPropertyChanged(const UnknownAddress& var) = 0;

};

NAMESPACE_END