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

	inline static void SingletonInitialize()
	{
		if constexpr (!std::is_abstract<T>())
			s_instance.reset(new T());
	}

	inline static void SingletonFinalize()
	{
		if constexpr (!std::is_abstract<T>())
			s_instance.reset(0);
	}

};

template <class T> std::unique_ptr<T> Singleton<T>::s_instance = nullptr;