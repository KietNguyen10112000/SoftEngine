#include "Model3DBasic.h"

#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

NAMESPACE_BEGIN

Model3DBasic::Model3DBasic(String path) : ResourceBase(path)
{
}

NAMESPACE_END