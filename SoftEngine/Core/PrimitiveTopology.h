#pragma once

#define DEFAULT_PRIMITIVE_TOPOLOGY PRIMITIVE_TOPOLOGY::TRIANGLE_LIST

#ifdef DX11Renderer
#include "../DX11/PrimitiveTopology.h"
#endif // DX11Renderer
