#include <gtest/gtest.h>

#include "Components/Renderers/Renderer.h"
#include "Object/GameObject.h"

#include "GTestLogger.h"

using namespace soft;

//TEST(EscTest, ComponentTest)
//{
//	class TestObject : public GameObject, Traceable<TestObject>
//	{
//	public:
//		Handle<GameObject> another = nullptr;
//
//	protected:
//		TRACEABLE_FRIEND();
//		void Trace(Tracer* tracer)
//		{
//			GameObject::Trace(tracer);
//			tracer->Trace(another);
//		}
//
//	};
//
//	class TestRenderer : public Renderer
//	{
//	public:
//		~TestRenderer()
//		{
//			GTestLogger::Log("~TestRenderer()");
//		}
//	};
//
//	class PlainComponent
//	{
//	public:
//		Vec4 v1;
//		Vec4 v2;
//		Mat4 m1;
//		Mat4 m2;
//
//	};
//
//	{
//		auto renderer = MakeShared<TestRenderer>();
//
//		SharedPtr<void> p = renderer;
//
//		auto obj = mheap::New<TestObject>();
//		obj->AddComponent(renderer);
//
//		auto obj2 = mheap::New<TestObject>();
//		obj->another = obj2;
//
//		obj2->AddComponent(renderer);
//		obj2->NewComponent<PlainComponent>();
//
//		auto testRenderer = obj2->GetComponent<TestRenderer>();
//		auto plainComp = obj2->GetComponent<PlainComponent>();
//		plainComp->m1.Rotation(Vec3::UP, PI / 2.0f);
//	}
//	gc::Run(-1);
//}