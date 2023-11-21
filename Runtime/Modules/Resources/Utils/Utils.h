#pragma once

#include "Core/Memory/Memory.h"
#include "Core/Structures/String.h"

#include "Scene/GameObject.h"

NAMESPACE_BEGIN

namespace ResourceUtils
{

API Handle<GameObject> LoadModel3DBasic(String path, String defaultDiffusePath = "");

API Handle<GameObject> LoadAnimModel(String path, String defaultDiffusePath = "");

}

NAMESPACE_END