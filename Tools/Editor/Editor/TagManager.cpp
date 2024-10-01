#include "TagManager.h"

#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"

#include "EditorContext.h"
#include "EditorTab.h"
#include "DataInspector.h"

#include "Scene/Scene.h"

bool TagManager::HasTag(const String& name)
{
	return m_hasTagName.find(name) != m_hasTagName.end();
}

bool TagManager::HasTag(size_t tagValue)
{
	return m_hasTagValue.find(tagValue) != m_hasTagValue.end();
}

bool TagManager::AddTag(size_t tagValue, const String& tagName)
{
	if (HasTag(tagValue))
	{
		return false;
	}

	m_hasTagName.insert(tagName);
	m_hasTagValue.insert(tagValue);
	m_tags.push_back({ tagValue, tagName });

	std::sort(m_tags.begin(), m_tags.end(), 
		[](const Tag& t0, const Tag& t1) 
		{
			return t0.value < t1.value;
		}
	);
}

void TagManager::RemoveTag(size_t tagValue)
{
	if (!HasTag(tagValue))
	{
		return;
	}

	auto it = std::find_if(m_tags.begin(), m_tags.end(), 
		[&](const Tag& t)
		{
			return t.value == tagValue;
		}
	);

	m_hasTagName.erase(it->name);
	m_hasTagValue.erase(it->value);
	m_tags.erase(it);
}

void TagManager::RemoveTag(const String& tagName)
{
	if (!HasTag(tagName))
	{
		return;
	}

	auto it = std::find_if(m_tags.begin(), m_tags.end(),
		[&](const Tag& t)
		{
			return t.name == tagName;
		}
	);

	m_hasTagName.erase(it->name);
	m_hasTagValue.erase(it->value);
	m_tags.erase(it);
}

void TagManager::ExportCppHeader()
{
	if (m_tags.empty())
	{
		return;
	}

	String fileName = m_cppHeaderExportFileName;
	if (fileName.empty())
	{
		return;
	}

	if (m_cppHeaderExportDir.empty())
	{
		return;
	}

	if (!EditorContext::Get()->IsVariableNameValid(fileName))
	{
		std::cerr << "[ERROR]: TagManager::ExportCppHeader(), invalid export  file name!\n";
		return;
	}

	auto exportPath = m_cppHeaderExportDir + m_cppHeaderExportFileName + ".h";

	String str = R"(#pragma once

class )";

	str += fileName + "\n{\npublic:\n\tenum ENUM\n\t{\n";

	for (auto& tag : m_tags)
	{
		str += "\t\t" + tag.name + " = " + String::From(tag.value) + ",\n";
	}

	str += "\t};\n};";

	if (FileSystem::Get()->IsFileExisted(exportPath) && FileSystem::Get()->IsFileChanged(exportPath))
	{
		byte* buffer = nullptr; size_t size = 0;
		FileUtils::ReadFile(exportPath, buffer, size);

		if (str == (char*)buffer)
		{
			FileUtils::FreeBuffer(buffer);
			return;
		}

		FileUtils::FreeBuffer(buffer);
	}

	std::cout << "[LOG]: TagManager::ExportCppHeader\n";
	FileUtils::WriteFile(exportPath.c_str(), str.c_str(), str.length());
}

void TagManager::WriteToJson(json& j)
{
	ExportCppHeader();

	auto& arr = j["Tags"];
	arr = json::array();
	for (auto& tag : m_tags)
	{
		auto& t = arr.emplace_back();
		t["Name"] = tag.name;
		t["Value"] = tag.value;
	}

	auto& jexport = j["TagsExport"];
	jexport["FileName"] = std::string(m_cppHeaderExportFileName);
	jexport["Directory"] = m_cppHeaderExportDir;
}

void TagManager::ReadFromJson(const json& j)
{
	auto& arr = j["Tags"];
	for (size_t i = 0; i < arr.size(); i++)
	{
		auto& t = arr[i];
		String name = t["Name"];
		ID value = t["Value"];
		AddTag(value, name);
	}

	if (j.contains("TagsExport"))
	{
		auto& jexport = j["TagsExport"];
		m_cppHeaderExportDir = jexport["Directory"];

		std::string name = jexport["FileName"];
		std::memcpy(m_cppHeaderExportFileName, name.data(), name.length() + 1);
	}
}

void TagManager::RenderSettingGUI()
{
	ImGui::TextUnformatted("Cpp Header Export");
	if (ImGui::BeginTable("Tags Export ## tags table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
	{
		auto width = 0.8f * ImGui::GetWindowWidth();

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("Directory");

		ImGui::TableNextColumn();
		Accessor temp = Accessor::ForString("Path", m_cppHeaderExportDir, nullptr);
		Variant var = Variant(VARIANT_TYPE::STRING_PATH);
		var.AsString() = m_cppHeaderExportDir;
		DataInspector::InspectStringPathEx(nullptr, temp, var, "Export Resource Path", true, width, true);

		ImGui::TableNextColumn();
		ImGui::TextUnformatted("File Name");

		ImGui::TableNextColumn();
		ImGui::SetNextItemWidth(width);
		ImGui::InputText("## Tags Export Input Text", m_cppHeaderExportFileName, sizeof(m_cppHeaderExportFileName));

		ImGui::EndTable();
	}

	ImGui::Dummy({ 15,15 });

	auto FnOnEndEditingName = [this](Tag& tag)
	{
		if (HasTag(m_inputBuf))
		{
			//std::cout << "[WARN]: TAG name already existed.\n";
		}
		else if (!EditorContext::Get()->IsVariableNameValid(m_inputBuf))
		{
			std::cerr << "[ERROR]: invalid TAG name.\n";
		}
		else
		{
			m_hasTagName.erase(tag.name);
			tag.name = m_inputBuf;
			m_hasTagName.insert(tag.name);
		}
	};

	auto FnOnEndEditingValue = [this](Tag& tag)
	{
		if (HasTag(m_inputTagValue))
		{
			//std::cout << "[WARN]: TAG value already existed.\n";
		}
		else if (m_inputTagValue < 0)
		{
			std::cout << "[WARN]: TAG value muset be >= 0.\n";
		}
		else
		{
			m_hasTagValue.erase(tag.value);
			tag.value = m_inputTagValue;
			m_hasTagValue.insert(tag.value);
		}
	};

	auto iter = EditorContext::Get()->GetCurrentTab()->GetScene()->GetIterationCount();
	if (iter != m_lastRenderSettingGUIIter + 1)
	{
		// reset state

		m_editingTagNameIdx = INVALID_ID;
		m_editingTagValueIdx = INVALID_ID;
		m_inputBuf[0] = 0;
		m_currentAddingTag.value = INVALID_ID;
		m_isAddingTag = false;
	}

	m_lastRenderSettingGUIIter = iter;

	if (ImGui::BeginTable("Tags ## tags table", 2, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
	{
		auto _w = ImGui::GetWindowWidth() - 100;
		auto width = _w * 0.75f;
		auto width2 = _w * 0.25f - 10;

		bool hovered = false;
		bool isEditSmth = false;

		ImGui::TableSetupColumn("Tag Name", 0, width);
		ImGui::TableSetupColumn("Tag Value", 0, width2);
		ImGui::TableHeadersRow();

		bool isAdding = false;
		if (m_isAddingTag)
		{
			m_tags.push_back(m_currentAddingTag);
			isAdding = true;
		}

		size_t i = 0;
		for (auto& tag : m_tags)
		{
			bool isAddingItem = isAdding && i == m_tags.size() - 1;

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(width);
			if (m_editingTagNameIdx == i)
			{
				if (ImGui::InputText("## tag name input", m_inputBuf, sizeof(m_inputBuf), ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue))
				{
					if (!isAddingItem)
					{
						FnOnEndEditingName(tag);
					}
					else
					{
						tag.name = m_inputBuf;
					}

					m_editingTagNameIdx = INVALID_ID;
				}

				if (ImGui::IsItemHovered())
				{
					hovered = true;
				}
			}
			else
			{
				auto str = tag.name + "## tag name";
				if (ImGui::Selectable(str.c_str()))
				{
					m_editingTagNameIdx = i;
					hovered = false;

					if (!tag.name.empty())
					{
						std::memcpy(m_inputBuf, tag.name.c_str(), sizeof(char) * (tag.name.length() + 1));
					}
					else
					{
						m_inputBuf[0] = 0;
					}
					
					isEditSmth = true;
				}
			}

			ImGui::TableNextColumn();
			ImGui::SetNextItemWidth(width2);
			if (m_editingTagValueIdx == i)
			{
				if (ImGui::InputInt("## tag value input", &m_inputTagValue, 0, 100, ImGuiInputTextFlags_::ImGuiInputTextFlags_CharsDecimal))
				{
					
				}

				if (ImGui::IsItemDeactivatedAfterEdit())
				{
					if (!isAddingItem)
					{
						FnOnEndEditingValue(tag);
					}
					else
					{
						tag.value = m_inputTagValue;
					}

					m_editingTagValueIdx = INVALID_ID;
				}

				if (ImGui::IsItemHovered())
				{
					hovered = true;
				}
			}
			else
			{
				String str = String::From(tag.value);
				if (ImGui::Selectable(str.c_str()))
				{
					m_editingTagValueIdx = i;
					hovered = true;
					m_inputTagValue = int(tag.value);
					isEditSmth = true;
				}
			}

			i++;
		}

		if (isAdding && (ImGui::IsKeyPressed(ImGuiKey_Enter) 
			|| (isEditSmth && m_editingTagValueIdx != m_tags.size() - 1 && m_editingTagNameIdx != m_tags.size() - 1)))
		{
			isAdding = false;
			m_currentAddingTag = m_tags.back();
			m_tags.pop_back();

			if (int(m_currentAddingTag.value) < 0)
			{
				std::cerr << "[ERROR]: TAG value must be >= 0.\n";
			}
			else if (!EditorContext::Get()->IsVariableNameValid(m_currentAddingTag.name))
			{
				std::cerr << "[ERROR]: invalid TAG name.\n";
			}
			else
			{
				AddTag(m_currentAddingTag.value, m_currentAddingTag.name);
			}

			m_currentAddingTag.value = INVALID_ID;
			m_isAddingTag = false;
		}
		
		if (!isAdding)
		{
			ImGui::TableNextColumn();
			if (m_currentAddingTag.value == INVALID_ID && ImGui::Button("+ New Tag"))
			{
				m_isAddingTag = true;
				m_currentAddingTag.name = "<Unnamed>";
				m_currentAddingTag.value = -1;
			}

			ImGui::TableNextColumn();
		}

		if (!hovered 
			&& (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1) || ImGui::IsMouseClicked(2)))
		{
			if (m_editingTagNameIdx < m_tags.size())
			{
				FnOnEndEditingName(m_tags[m_editingTagNameIdx]);
			}

			if (m_editingTagValueIdx < m_tags.size())
			{
				FnOnEndEditingValue(m_tags[m_editingTagValueIdx]);
			}

			m_editingTagNameIdx = INVALID_ID; 
			m_editingTagValueIdx = INVALID_ID;
		}

		if (isAdding)
		{
			m_currentAddingTag = m_tags.back();
			m_tags.pop_back();
		}

		ImGui::EndTable();
	}
	
}

void TagManager::RenderTagInput(GameObject* obj, ClassMetadata* metadata)
{
	if (m_inputingTagObj != obj)
	{
		auto objTag = obj->Tag();

		m_inputTagBuf[0] = 0;
		m_inputingTagObj = obj;
		m_inputTagOpenState = 0;
		m_inputTagOpenStateCount = 0;
		m_isInputingTagName = false;
		m_isInputingTagValue = false;

		for (auto& tag : m_tags)
		{
			if (tag.value == objTag)
			{
				m_currentInputObjectTag = tag;
				break;
			}
		}
	}

	if (!ImGui::BeginTable("Tags Export ## tags table", 3, ImGuiTableFlags_Borders | ImGuiTableFlags_SizingFixedFit))
	{
		return;
	}

	auto width = ImGui::GetWindowWidth() * 0.75f - 50;
	auto width2 = ImGui::GetWindowWidth() * 0.25f;

	ImGui::TableSetupColumn("", 0, 50);
	ImGui::TableSetupColumn("Tag Name", 0, width);
	ImGui::TableSetupColumn("Tag Value", 0, width2);
	//ImGui::TableHeadersRow();

	ImGui::TableNextColumn();
	ImGui::TextUnformatted("Tag");
	ImGui::TableSetBgColor(ImGuiTableBgTarget_::ImGuiTableBgTarget_CellBg, IM_COL32(100, 100, 100, 255));

	ImGui::TableNextColumn();
	ImGui::SetNextItemWidth(width);
	if (m_isInputingTagName)
	{
		if (ImGui::InputText("##input", m_inputTagBuf, sizeof(m_inputTagBuf), ImGuiInputTextFlags_::ImGuiInputTextFlags_EnterReturnsTrue))
		{
			m_isInputingTagName = false;
		}

		if (m_inputTagOpenState == 0 && ImGui::IsItemFocused() && ImGui::IsItemActive())
		{
			m_inputTagOpenState = 1;
		}

		if (m_inputTagOpenState == 2 && ImGui::IsItemHovered() && ImGui::IsMouseClicked(0))
		{
			m_inputTagOpenState = 0;
		}

		bool isInputTextHovered = ImGui::IsItemHovered();
		if (!ImGui::IsItemHovered() && (ImGui::IsMouseClicked(0) || ImGui::IsMouseClicked(1)))
		{
			m_inputTagOpenStateCount = 20;
		}

		if (m_inputTagOpenStateCount != 0)
		{
			if (--m_inputTagOpenStateCount == 0)
			{
				m_inputTagOpenState = 0;
			}
		}

		if (m_inputTagOpenState == 1)
		{
			ImGui::SetNextWindowPos({ ImGui::GetItemRectMin().x, ImGui::GetItemRectMax().y });
			ImGui::SetNextWindowSize({ ImGui::GetItemRectSize().x, 0 });
			ImGui::SetNextWindowSizeConstraints({ 0, 0 }, { ImGui::GetItemRectSize().x, 500 });
			if (ImGui::Begin("##popup", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_Tooltip))
			{
				ImGui::BringWindowToDisplayFront(ImGui::GetCurrentWindow());
				//static const char* autocomplete[] = { "cats", "dogs", "rabbits", "turtles" };
				for (int i = 0; i < m_tags.size(); i++)
				{
					auto autocomplete = m_tags[i].name.c_str();

					if (strstr(autocomplete, m_inputTagBuf) == NULL)
						continue;

					if (ImGui::Selectable(autocomplete))
					{
						strcpy(m_inputTagBuf, autocomplete);
						m_inputTagOpenState = 2;
						m_isInputingTagName = false;
					}
				}
			}

			ImGui::End();
		}

		if (m_isInputingTagName == false)
		{
			String tagName = m_inputTagBuf;
			if (tagName.empty() || !HasTag(tagName))
			{
				//std::cerr << "[ERROR]: invalid tag name.\n";
				*(ID*)&obj->Tag() = INVALID_ID;
			}
			else
			{
				for (auto& tag : m_tags)
				{
					if (tag.name == tagName)
					{
						m_currentInputObjectTag = tag;
						*(ID*)&obj->Tag() = tag.value;
						break;
					}
				}
			}
		}
	}
	else
	{
		String str = obj->Tag() == INVALID_ID ? "<Untagged>" : m_currentInputObjectTag.name;

		if (ImGui::Selectable(str.c_str()) && m_isInputingTagValue == false)
		{
			m_isInputingTagName = true;

			m_inputTagBuf[0] = 0;
			m_inputTagOpenState = 0;
			m_inputTagOpenStateCount = 0;
		}
	}

	//ImGui::SameLine(ImGui::GetWindowWidth() * 0.8f);

	ImGui::TableNextColumn();
	if (m_isInputingTagValue)
	{

	}
	else
	{
		String str = obj->Tag() == INVALID_ID ? "---" : String::From(obj->Tag());
		auto posX = (ImGui::GetCursorPosX() + ImGui::GetColumnWidth() / 2.0f - ImGui::CalcTextSize(str.c_str()).x / 2.0f
			- ImGui::GetScrollX());
		if (posX > ImGui::GetCursorPosX())
			ImGui::SetCursorPosX(posX);

		ImGui::TextUnformatted(str.c_str());
	}

	ImGui::EndTable();
	
}