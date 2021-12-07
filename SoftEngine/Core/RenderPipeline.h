#pragma once

#ifdef DX11_RENDERER
#include "../RenderAPI/DX11/RenderPipeline.h"
#endif

class RenderPipelineManager
{
private:
	using MapType = std::map<_RenderPipelineKey, RenderPipeline*,
		bool(*)(const _RenderPipelineKey&, const _RenderPipelineKey&)>;
	MapType m_map;

	using CacheMapType = std::map<std::wstring, std::pair<RenderPipeline*, uint32_t>*>;
	CacheMapType m_cache;

	std::mutex m_lock;

public:
	RenderPipelineManager();
	~RenderPipelineManager();

private:
	RenderPipeline* _Get(const char* inputLayoutSource, const std::wstring& vs, const std::wstring& ps);

	RenderPipeline* _Get(const char* inputLayoutSource, const std::wstring& vs,
		const std::wstring& gs, const std::wstring& ps);
	RenderPipeline* _Get(const char* inputLayoutSource, const std::wstring& vs,
		const std::wstring& hs, const std::wstring& ds, const std::wstring& gs, const std::wstring& ps);

	void _Release(RenderPipeline** rpl);

	RenderPipeline* _Get(RenderPipeline* rpl);

public:
	/*eg inputLayoutSource:
	struct Vertex
	{
		Vec3 pos; POSITION, PER_VERTEX #
		Vec2 textCoord; TEXTCOORD, PER_VERTEX #
	};*/
	static RenderPipeline* Get(const char* inputLayoutSource, const std::wstring& vs, const std::wstring& ps);

	static RenderPipeline* Get(const char* inputLayoutSource, const std::wstring& vs,
		const std::wstring& gs, const std::wstring& ps);
	static RenderPipeline* Get(const char* inputLayoutSource, const std::wstring& vs,
		const std::wstring& hs, const std::wstring& ds, const std::wstring& gs, const std::wstring& ps);

	static void Release(RenderPipeline** rpl);

	static RenderPipeline* Get(RenderPipeline* rpl);

};