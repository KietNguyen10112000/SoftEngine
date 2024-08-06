#pragma once
#include <vector>

#include "Core/Memory/Memory.h"

#include "EditorContext.h"

using namespace soft;

namespace soft
{
	class GameObject;
	class Scene;
}

class EditorTab
{
private:
	friend class EditorContext;

	bool m_isShowing = false;
	bool m_isFirstShow = true;
	bool m_isPlacedHolder = false;
	bool m_padd[1];

	String m_saveDirectory = "";

public:
	ID m_id = INVALID_ID;
	String m_name = "<Unnamed>";
	Scene* m_scene = nullptr;

private:
	inline void Show()
	{
		m_isShowing = true;
		if (m_isFirstShow)
		{
			std::cout << "Open\n";
			OnOpen();
			m_isFirstShow = false;
		}

		std::cout << "Show\n";
		OnShow();
	}

	inline void Hide()
	{
		std::cout << "Hide\n";
		m_isShowing = false;
		OnHide();
	}

	inline void Close()
	{
		std::cout << "Hide\n";
		std::cout << "Close\n";
		OnHide();
		OnClose();
	}

public:
	virtual void OnObjectsAdded(std::vector<GameObject*>& objects) = 0;
	virtual void OnObjectsRemoved(std::vector<GameObject*>& objects) = 0;
	virtual void OnRenderGUI() = 0;
	virtual void OnRenderInGameDebugGraphics() = 0;
	inline virtual void OnRenderMenuBar(const String& menuName) {};

	virtual void OnShow() = 0;
	virtual void OnHide() = 0;

	virtual void OnOpen() = 0;
	virtual void OnClose() = 0;

	virtual String GetTabClassName() const = 0;

	void SetSaveFilePath(const String& dir);

	inline String GetSaveFilePath() const
	{
		if (m_saveDirectory.empty())
		{
			return m_saveDirectory;
		}

		return m_saveDirectory + m_name + "." + GetTabClassName();
	}

	inline virtual bool IsCloseable() { return true; };

	inline auto GetScene()
	{
		return m_scene;
	}

	inline bool IsShowing() const
	{
		return m_isShowing;
	}

};

