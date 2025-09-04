#pragma once

#ifndef UNICODE
#define UNICODE
#endif

#define WIIN32_LEAN_AND_MEAN
#include <windows.h>
#include <shellapi.h>

#if defined(min)
#undef min
#endif

#if defined(max)
#undef max
#endif

#if defined(CreateWindow)
#undef CreateWindow	
#endif

#include <wrl.h>
using namespace Microsoft::WRL;
#include <d3d12.h>
#include <dxgi1_6.h>
#include <d3dcompiler.h>
#include <DirectXMath.h>

#include <d3dx12.h>

#include <cassert>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <map>

#include <Helper.h>
