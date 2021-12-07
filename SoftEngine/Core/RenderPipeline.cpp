#include <RenderPipeline.h>

#ifdef DX11_RENDERER
#include "../RenderAPI/DX11/RenderPipeline.cpp"
#endif

#include <Engine/Global.h>

#include <Shader.h>
#include <Resource.h>

#define ___RPL_GET_IMP_(className) \
WaitToLock(m_lock);\
auto it = m_map.find(key);\
if (it == m_map.end())\
{\
	RenderPipeline* re = new className();\
	key.vs->SetInputLayout(inputLayoutSource);\
	re->Config(&key);\
	m_map.insert({ key, re });\
	::Release(m_lock);\
	return re;\
}\
else\
{\
	it->second->m_refCount++;\
	RenderPipeline* re = it->second;\
	::Release(m_lock);\
	Resource::Release(&key.vs);\
	Resource::Release(&key.ps);\
	Resource::Release(&key.gs);\
	Resource::Release(&key.hs);\
	Resource::Release(&key.ds);\
	return re;\
}

RenderPipelineManager::RenderPipelineManager()
{
	m_map = MapType(
		[](const _RenderPipelineKey& a, const _RenderPipelineKey& b) -> bool
		{
			return ::memcmp(&a, &b, sizeof(_RenderPipelineKey)) > 0;
		}
	);
}

RenderPipelineManager::~RenderPipelineManager()
{
}

RenderPipeline* RenderPipelineManager::_Get(const char* inputLayoutSource, const std::wstring& vs, const std::wstring& ps)
{
	_RenderPipelineKey key = {};

	key.vs = Resource::Get<VertexShader>(vs);
	key.ps = Resource::Get<PixelShader>(ps);

	___RPL_GET_IMP_(RenderPipeline);
}

RenderPipeline* RenderPipelineManager::Get(const char* inputLayoutSource, const std::wstring& vs, const std::wstring& ps)
{
	return Global::rplManager->_Get(inputLayoutSource, vs, ps);
}

RenderPipeline* RenderPipelineManager::_Get(const char* inputLayoutSource, const std::wstring& vs, 
	const std::wstring& gs, const std::wstring& ps)
{
	_RenderPipelineKey key = {};

	key.vs = Resource::Get<VertexShader>(vs);
	key.ps = Resource::Get<PixelShader>(ps);
	key.gs = Resource::Get<GeometryShader>(ps);

	___RPL_GET_IMP_(RenderPipeline1);
}

RenderPipeline* RenderPipelineManager::Get(const char* inputLayoutSource, const std::wstring& vs, 
	const std::wstring& gs, const std::wstring& ps)
{
	return Global::rplManager->_Get(inputLayoutSource, vs, gs, ps);
}

void RenderPipelineManager::_Release(RenderPipeline** rpl)
{
	if (!(*rpl)) return;
	_RenderPipelineKey key = (*rpl)->GetKey();

	(*rpl) = nullptr;

	WaitToLock(m_lock);

	auto it = m_map.find(key);

	if (it != m_map.end())
	{
		it->second->m_refCount--;
		
		if (it->second->m_refCount == 0)
		{
			delete it->second;
			m_map.erase(it);
		}
	}

	::Release(m_lock);
}



void RenderPipelineManager::Release(RenderPipeline** rpl)
{
	Global::rplManager->_Release(rpl);
}

RenderPipeline* RenderPipelineManager::_Get(RenderPipeline* rpl)
{
	WaitToLock(m_lock);
	rpl->m_refCount++;
	::Release(m_lock);
	return rpl;
}

RenderPipeline* RenderPipelineManager::Get(RenderPipeline* rpl)
{
	return Global::rplManager->_Get(rpl);
}
