#include "Plugins/Bridge/PluginImpl.h"

#include "MyTestScript.h"

void RegisterSerializables()
{
	// registers all custom serializable classes, includes all scripts, all custom components like child classes of RenderingComponent,...
	// eg:
	//	SerializableDB::Get()->Register<MyScript1>();
	//	SerializableDB::Get()->Register<MyScript2>();
	//	...
	//	SerializableDB::Get()->Register<<custom serializable classes ClassName>>();

	SerializableDB::Get()->Register<MyTestScript>();
}

void Initialize(Runtime* runtime)
{
	std::cout << "Hello, World from Test!\n";
}

void Finalize(Runtime* runtime)
{

}