#pragma once

#include "Resource.h"

#include "Graphics/Graphics.h"
#include "FileSystem/FileSystem.h"

#include "Model3DBasic.h"

NAMESPACE_BEGIN

// static model 3d
class Model3D : public Model3DBasic
{
public:
	struct Node
	{
		String name;
		ID parentId;
		ID meshId;
		Transform localTransform;
	};

	std::vector<Node> m_nodes;

	Model3D(String path);

	virtual Handle<GameObject> MakeGameObject() override;
};

NAMESPACE_END