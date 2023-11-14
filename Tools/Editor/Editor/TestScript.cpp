#include "TestScript.h"

#include "Input/Input.h"

//#include "MainSystem/Rendering/Components/Model3DBasicRenderer.h"

#include "RotateScript.h"

void TestScript::OnStart()
{
}

void TestScript::OnUpdate(float dt)
{
	/*if (Input()->IsKeyUp('P'))
	{
		auto obj = mheap::New<GameObject>();
		obj->NewComponent<RotateScript>();
		obj->NewComponent<Model3DBasicRenderer>("cube.obj", "2.png");
		obj->Name() = "Parent";

		{
			auto child = mheap::New<GameObject>();
			child->NewComponent<RotateScript>();
			child->NewComponent<Model3DBasicRenderer>("cube.obj", "2.png");

			Transform transform = {};
			transform.Position() = { 0, 3, 0 };
			child->SetLocalTransform(transform);
			child->Name() = "Child";
			obj->AddChild(child);
		}

		GetScene()->AddObject(obj);

		m_obj = obj.Get();
	}*/

	/*if (Input()->IsKeyUp('O') && s_obj)
	{
		s_obj->RemoveComponent<RotateScript>(nullptr);
	}

	if (Input()->IsKeyUp('I') && s_obj)
	{
		s_obj->NewComponent<RotateScript>();
	}*/

	/*if (Input()->IsKeyUp(KEYBOARD::SPACE) && m_obj)
	{
		auto child = mheap::New<GameObject>();
		child->NewComponent<RotateScript>();
		child->NewComponent<Model3DBasicRenderer>("cube.obj", "2.png");

		child->SetLocalTransform(m_childTransform);
		child->Name() = String::Format("Child {}", (++m_childCount));
		m_obj->AddChild(child);
	}*/

	/*if (Input()->IsKeyUp('T') && m_obj)
	{
		auto child = m_obj->Children()[0];
		child->RemoveFromParent();
		GetScene()->AddObject(child);
	}*/

}

Handle<ClassMetadata> TestScript::GetMetadata(size_t sign)
{
	auto metadata = ClassMetadata::For(this);

	metadata->AddProperty(Accessor::For("Child transform", m_childTransform, this));

	return metadata;
}
