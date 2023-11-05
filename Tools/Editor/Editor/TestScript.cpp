#include "TestScript.h"

#include "Input/Input.h"

#include "MainSystem/Rendering/Components/Model3DBasicRenderer.h"

#include "RotateScript.h"

void TestScript::OnStart()
{
}

void TestScript::OnUpdate(float dt)
{
	if (Input()->IsKeyUp('P'))
	{
		std::cout << "TestScript --- IsKeyUp\n";
		auto obj = mheap::New<GameObject>();
		obj->NewComponent<RotateScript>();
		obj->NewComponent<Model3DBasicRenderer>("cube.obj", "2.png");
		GetScene()->AddObject(obj);
	}
}
