#pragma once

class v8EngineWrapper;
class Engine;
class Scene;

void SoftWrapToV8(v8EngineWrapper* v8engine, Engine* softEngine, Scene* softScene);