#pragma once

namespace soft
{
	class MainComponent;
}

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

};