#include <yvals_core.h>

#define ThrowIfFailed(hr, msg) if (FAILED(hr)) throw msg L"\nThrow from File \"" __FILEW__ L"\", Line " _STRINGIZE(__LINE__) L"."