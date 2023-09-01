#pragma once

#include <memory>

template <typename T>
class Singleton
{
protected:
	static std::unique_ptr<T> s_instance;

public:
	inline static T* Get()
	{
		return s_instance.get();
	}

};

template <class T> std::unique_ptr<T> Singleton<T>::s_instance = nullptr;