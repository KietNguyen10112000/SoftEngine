#pragma once

namespace soft
{
	class MainComponent;
	class GameObject;
}

using namespace soft;

class ComponentInspectorBase
{
private:
	friend class ComponentInspector;
	void* m_boundComp = nullptr;

public:
	inline ComponentInspectorBase(soft::MainComponent* p) : m_boundComp(p) {};
	virtual ~ComponentInspectorBase() {};

public:
	virtual void OnBeginInspecting() = 0;
	virtual void OnEndInspecting() = 0;

public:
	static void SetOpacityForObject(GameObject* o, float alpha);

};