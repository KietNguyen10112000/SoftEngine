#pragma once

#include "EditorTab.h"

#include <map>

class EditorTabFactory
{
private:
	friend class EditorContext;
	bool m_overwriteExist = false;
	String m_pathToCheckExist = "";

public:
	String m_tabKindName;

	inline virtual ~EditorTabFactory() {};

	virtual void Begin() = 0;
	virtual void End() = 0;

	virtual void ShowCreationInputGUI() = 0;
	virtual Handle<EditorTab> CreateInstance() = 0;

	bool AskIfExisted(const String& path);
};

class EditorTabFactoryManager : public Singleton<EditorTabFactoryManager>
{
private:
	std::map<String, EditorTabFactory*> m_map;

public:
	EditorTabFactoryManager();
	~EditorTabFactoryManager();

private:
	template <typename T>
	inline void RegisterFactory()
	{
		static_assert(std::is_base_of_v<EditorTabFactory, T>);
		m_map[typeid(T).name()] = new T();
	}

public:
	template <typename T>
	inline T* GetFactory()
	{
		static_assert(std::is_base_of_v<EditorTabFactory, T>);
		return (T*)m_map[typeid(T).name()];
	}

	template <typename Fn>
	inline void ForEach(Fn callback)
	{
		for (auto& [key, value] : m_map)
		{
			callback(key, value);
		}
	}

};