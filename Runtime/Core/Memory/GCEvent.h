#pragma once

#include "TypeDef.h"

NAMESPACE_MEMORY_BEGIN

class GCEvent
{
public:
	virtual ~GCEvent() {};

public:
	virtual void OnMarkBegin()		= 0;
	virtual void OnMarkEnd()		= 0;

	virtual void OnRemarkBegin()	= 0;
	virtual void OnRemarkEnd()		= 0;

	virtual void OnSweepBegin()		= 0;
	virtual void OnSweepEnd()		= 0;

};

NAMESPACE_MEMORY_END