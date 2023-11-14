#pragma once

#include "Resource.h"

#include "Graphics/Graphics.h"
#include "FileSystem/FileSystem.h"

#include "MeshBasic.h"

NAMESPACE_BEGIN

class MeshBasic;

// basic model3d with vertex and uv (textcoord), all static meshes
class Model3DBasic : public ResourceBase
{
public:
	std::vector<Resource<MeshBasic>> m_meshes;

	Model3DBasic(String path);

};

NAMESPACE_END