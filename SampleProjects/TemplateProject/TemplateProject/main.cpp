#include "Plugins/Bridge/PluginImpl.h"

void RegisterCustomMainComponents()
{
	// registers all custom main components, includes all scripts
	// eg:
	//	MainComponentDB::Get()->RegisterComponent<MyScript1>();
	//	MainComponentDB::Get()->RegisterComponent<MyScript2>();
	//	...
	//	MainComponentDB::Get()->RegisterComponent<<custom components ClassName>>();
}

void Initialize(Runtime* runtime)
{
	std::cout << "Hello, World from TemplateProject!\n";
}

void Finalize(Runtime* runtime)
{

}