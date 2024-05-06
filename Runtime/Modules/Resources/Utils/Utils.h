#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Memory/SmartPointers.h"
#include "Core/Structures/String.h"

#include "Scene/GameObject.h"

#include "../AnimMotion.h"

NAMESPACE_BEGIN

namespace ResourceUtils
{

//API Handle<GameObject> LoadModel3DBasic(String path, String defaultDiffusePath = "", bool placeHolder = false);

//API Handle<GameObject> LoadAnimModel(String path, String defaultDiffusePath = "");

//API Handle<GameObject> LoadAnimModelArray(String path, String defaultDiffusePath = "", bool placeHolder = false);

//API std::vector<Resource<AnimMotion>> LoadAnimMotion(String path);

}

NAMESPACE_END