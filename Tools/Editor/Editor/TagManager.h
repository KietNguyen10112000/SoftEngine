#pragma once

#include "Core/Pattern/Singleton.h"
#include "Core/Structures/String.h"

#include "JSON/JSON.h"

using namespace soft;

namespace soft
{
	class GameObject;
	class ClassMetadata;
}

class TagManager : public Singleton<TagManager>
{
private:
	struct Tag
	{
		size_t value;
		String name;
	};

	std::vector<Tag> m_tags;

	std::set<String> m_hasTagName;
	std::set<size_t> m_hasTagValue;

	char m_inputBuf[256] = {};
	ID m_editingTagNameIdx = INVALID_ID;
	ID m_editingTagValueIdx = INVALID_ID;
	int m_inputTagValue = 0;
	size_t m_lastRenderSettingGUIIter = 0;

	Tag m_currentAddingTag = {};
	bool m_isAddingTag = false;

	char m_cppHeaderExportFileName[256] = {};
	String m_cppHeaderExportDir = "";

	GameObject* m_inputingTagObj = nullptr;
	char m_inputTagBuf[256] = {};
	byte m_inputTagOpenState = 0;
	byte m_inputTagOpenStateCount = 0;

	bool m_isInputingTagName = false;
	Tag m_currentInputObjectTag = {};

	bool m_isInputingTagValue = false;

public:
	bool HasTag(const String& tagName);
	bool HasTag(size_t tagValue);
	bool AddTag(size_t tagValue, const String& tagName);
	void RemoveTag(size_t tagValue);
	void RemoveTag(const String& tagName);

	void ExportCppHeader();
	void WriteToJson(json& j);
	void ReadFromJson(const json& j);

	void RenderSettingGUI();

	void RenderTagInput(GameObject* obj, ClassMetadata* metadata);
};