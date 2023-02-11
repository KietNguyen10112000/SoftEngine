#include "BuiltinEvent.h"

NAMESPACE_BEGIN

class Scene;
class GameObject;

#define IMPL_EVENT(name, scene, obj) void ___IMPL_EVENT_ ## name (Scene* scene, GameObject* obj)
#define IMPL_EVENT_FUNC(name) ___IMPL_EVENT_ ## name

#define CALL_EVENT(event, name) \
InvokeEvent<event, name>(objEvents, scene);


#define CALL_ALL_EVENTS()																				\
CALL_EVENT(BUILTIN_EVENT::SCENE_ADD_OBJECT		, IMPL_EVENT_FUNC(SCENE_ADD_OBJECT)		);				\
CALL_EVENT(BUILTIN_EVENT::SCENE_REMOVE_OBJECT	, IMPL_EVENT_FUNC(SCENE_REMOVE_OBJECT)	);


IMPL_EVENT(SCENE_ADD_OBJECT, scene, obj)
{
	std::cout << "SCENE_ADD_OBJECT\n";
}

IMPL_EVENT(SCENE_REMOVE_OBJECT)
{
	std::cout << "SCENE_REMOVE_OBJECT\n";
}


NAMESPACE_END