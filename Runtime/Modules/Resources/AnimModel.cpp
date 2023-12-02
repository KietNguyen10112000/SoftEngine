#include "AnimModel.h"

NAMESPACE_BEGIN

const char* AnimModel::CACHE_EXTENSION = ".AnimModel";

AnimModel::AnimModel(String path, bool placeholder) : Model3DBasic(path, true)
{
}

NAMESPACE_END