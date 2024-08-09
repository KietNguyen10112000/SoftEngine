#pragma once

#include "TestScript.h"
#include "RotateScript.h"

#include "AnimatorEditorSaveData.h"

#include "Common/Base/SerializableDB.h"

#include "AnimatorEditorTab.h"
#include "SceneEditorSaveData.h"
#include "GameObjectEditorSaveData.h"

inline void InitializeScriptList()
{
	SerializableDB::Get()->Register<TestScript>();
	SerializableDB::Get()->Register<RotateScript>();

	SerializableDB::Get()->Register<AnimatorEditorSaveData>();
	SerializableDB::Get()->Register<SceneEditorSaveData>();
	SerializableDB::Get()->Register<GameObjectEditorSaveData>();

	AnimatorEditorTab::InitializeSerializableList();
}