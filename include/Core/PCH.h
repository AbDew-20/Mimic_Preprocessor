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

#ifndef DWORD_MAX
#define DWORD_MAX ((DWORD)0xFFFFFFFF)
#endif

#include <directx/d3dx12.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <DirectXMath.h>


#include <cassert>
#include <chrono>
#include <algorithm>
#include <iostream>
#include <map>

#include <Core/Helper.h>
#include <Core/Clock.h>
#include <Core/ScopedTimer.h>
