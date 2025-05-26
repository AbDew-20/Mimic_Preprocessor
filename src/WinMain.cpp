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

#include <Helper.h>

const uint8_t g_NumFrames = 2;
bool g_UseWarp = false;

uint32_t g_ClientWidth = 1280;
uint32_t g_ClientHeight = 720;

bool g_IsInitialised = false;

HWND g_hwnd;

RECT g_windowRect;

ComPtr<ID3D12Device2> g_Device;
ComPtr<ID3D12CommandQueue> g_CommandQueue;
ComPtr<IDXGISwapChain4> g_SwapChain;
ComPtr<ID3D12Resource> g_BackBuffers[g_NumFrames];
ComPtr<ID3D12GraphicsCommandList> g_CommandList;
ComPtr<ID3D12CommandAllocator> g_CommandAllocators[g_NumFrames];
ComPtr<ID3D12DescriptorHeap> g_RTVDescriptorHeap;
UINT g_RTVDescriptorSize;
UINT g_CurrentBackBufferIndex;

ComPtr<ID3D12Fence> g_Fence;
uint64_t g_FenceValue = 0;
uint64_t g_FrameFenceValues[g_NumFrames] = {};
HANDLE g_FenceEvent;

bool g_Vsync = true;
bool g_TearingSupported = false;
bool g_Fullscreen = false;

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);

void ParseCommandLineArguments(){
	int argc;
	wchar_t **argv = ::CommandLineToArgvW(::GetCommandLineW(), &argc);
	for(size_t i = 0; i<argc; ++i){
		if(::wcscmp(argv[i], L"-w")==0||::wcscmp(argv[i], L"--width")==0){
			g_ClientWidth = ::wcstol(argv[++i], nullptr, 10);
		}
		if(::wcscmp(argv[i], L"-h")==0||::wcscmp(argv[i], L"--height")==0){
			g_ClientHeight = ::wcstol(argv[++i], nullptr, 10);
		}
		if(::wcscmp(argv[i], L"-warp")==0||::wcscmp(argv[i], L"--warp")==0){
			g_UseWarp = true;
		}
	}
	::LocalFree(argv);
}

void EnableDebugLayer(){
#if defined(_DEBUG)
	ComPtr<ID3D12Debug> debugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&debugInterface)));
	debugInterface->EnableDebugLayer();
#endif

}

void RegisterWindowClass(HINSTANCE hInstance, const wchar_t *windowClassName){
	WNDCLASSEXW windClass = {};
	windClass.cbSize = sizeof(WNDCLASSEXW);
	windClass.style = CS_HREDRAW||CS_VREDRAW;
	windClass.lpfnWndProc = WindowProc;
	windClass.hInstance = hInstance;
	windClass.lpszClassName = windowClassName;
	windClass.lpszMenuName = NULL;
	windClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	static ATOM atom = ::RegisterClassExW(&windClass);

	assert(atom>0);

}

HWND CreateWindow(const wchar_t *windowClassName, HINSTANCE hInstance, const wchar_t *windowTitle, const uint32_t width, const uint32_t height){
	int screenWidth = ::GetSystemMetrics(SM_CXSCREEN);
	int screenHeight = ::GetSystemMetrics(SM_CYSCREEN);
	RECT windRect = {0,0,static_cast<LONG>(width),static_cast<LONG>(height)};
	::AdjustWindowRect(&windRect, WS_OVERLAPPEDWINDOW, FALSE);
	int windowWidth = windRect.right-windRect.left;
	int windowHeight = windRect.bottom-windRect.top;

	int windowX = std::max<int>(0, (screenWidth-windowWidth)/2);
	int windowY = std::max<int>(0, (screenHeight-windowHeight)/2);

	HWND hWnd = ::CreateWindowExW(NULL, windowClassName, windowTitle, WS_OVERLAPPEDWINDOW, windowX, windowY, windowWidth, windowHeight, NULL, NULL, hInstance, nullptr);
	assert(hWnd&&"Failed to create window");
	return hWnd;
}

ComPtr<IDXGIAdapter4> Getadapter(bool useWarp){
	ComPtr<IDXGIFactory4> dxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory)));
	ComPtr<IDXGIAdapter1> dxgiAdapter1;
	ComPtr<IDXGIAdapter4> dxgiAdapter4;

	if(useWarp){
		ThrowIfFailed(dxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&dxgiAdapter1)));
		ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));

	}
	else{
		SIZE_T maxDedicatedMemory = 0;
		for(UINT i = 0; dxgiFactory->EnumAdapters1(i, &dxgiAdapter1)!=DXGI_ERROR_NOT_FOUND; ++i){
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			dxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);

			if((dxgiAdapterDesc1.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)==0&&SUCCEEDED(D3D12CreateDevice(dxgiAdapter1.Get(), D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))&&dxgiAdapterDesc1.DedicatedVideoMemory>maxDedicatedMemory){
				maxDedicatedMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(dxgiAdapter1.As(&dxgiAdapter4));

			}
		}
	}
	return dxgiAdapter4;

}

ComPtr<ID3D12Device2> CreateDevice(ComPtr<IDXGIAdapter4> adapter){
	ComPtr<ID3D12Device2> d3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(adapter.Get(), D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&d3d12Device2)));

#if defined(_DEBUG)
	ComPtr<ID3D12InfoQueue> pInfoQueue;
	if(SUCCEEDED(d3d12Device2.As(&pInfoQueue))){
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_CORRUPTION, TRUE);
		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_ERROR, TRUE);

		pInfoQueue->SetBreakOnSeverity(D3D12_MESSAGE_SEVERITY_WARNING, TRUE);

		D3D12_MESSAGE_SEVERITY Severities[] = {
			D3D12_MESSAGE_SEVERITY_INFO
		};
		D3D12_MESSAGE_ID DenyIds[] = {
			D3D12_MESSAGE_ID_CLEARRENDERTARGETVIEW_MISMATCHINGCLEARVALUE,
			D3D12_MESSAGE_ID_MAP_INVALID_NULLRANGE,
			D3D12_MESSAGE_ID_UNMAP_INVALID_NULLRANGE
		};
		D3D12_INFO_QUEUE_FILTER NewFilter = {};
		NewFilter.DenyList.NumSeverities = _countof(Severities);
		NewFilter.DenyList.pSeverityList = Severities;
		NewFilter.DenyList.NumIDs = _countof(DenyIds);
		NewFilter.DenyList.pIDList = DenyIds;

		ThrowIfFailed(pInfoQueue->PushStorageFilter(&NewFilter));
	}
#endif
	return d3d12Device2;
}

ComPtr<ID3D12CommandQueue> CreateCommandQueue(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type){
	ComPtr<ID3D12CommandQueue> d3d12CommandQueue;
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = type;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(device->CreateCommandQueue(&desc, IID_PPV_ARGS(&d3d12CommandQueue)));

	return d3d12CommandQueue;
}

ComPtr<IDXGISwapChain4> CreateSwapChain(HWND hwnd, ComPtr<ID3D12CommandQueue> commandQueue, uint32_t width, uint32_t height, uint32_t bufferCount){
	ComPtr<IDXGISwapChain4> dxgiSwapChain;
	ComPtr<IDXGIFactory4> dxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif

	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&dxgiFactory4)));
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = width;
	swapChainDesc.Height = height;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = {1,0};
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = bufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = 0;

	ComPtr<IDXGISwapChain1> swapChain1;
	ThrowIfFailed(dxgiFactory4->CreateSwapChainForHwnd(commandQueue.Get(), hwnd, &swapChainDesc, nullptr, nullptr, &swapChain1));
	ThrowIfFailed(dxgiFactory4->MakeWindowAssociation(hwnd, DXGI_MWA_NO_ALT_ENTER));

	ThrowIfFailed(swapChain1.As(&dxgiSwapChain));

	return dxgiSwapChain;

}

ComPtr<ID3D12DescriptorHeap> CreateDescriptorHeap(ComPtr<ID3D12Device2> device, D3D12_DESCRIPTOR_HEAP_TYPE type, uint32_t numDescriptors){
	ComPtr<ID3D12DescriptorHeap> d3d12DescriptorHeap;
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.NumDescriptors = numDescriptors;
	desc.Type = type;
	ThrowIfFailed(device->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&d3d12DescriptorHeap)));
	return d3d12DescriptorHeap;

}

void UpdateRenderTargetViews(ComPtr<ID3D12Device2> device, ComPtr<IDXGISwapChain4> dxgiSwapChain, ComPtr<ID3D12DescriptorHeap> descriptorHeap){
	auto rtvDescriptorSize = device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(descriptorHeap->GetCPUDescriptorHandleForHeapStart());

	for(int i = 0; i<g_NumFrames; ++i){
		ComPtr<ID3D12Resource> backBuffer;
		ThrowIfFailed(dxgiSwapChain->GetBuffer(i, IID_PPV_ARGS(&backBuffer)));
		device->CreateRenderTargetView(backBuffer.Get(), nullptr, rtvHandle);
		g_BackBuffers[i] = backBuffer;
		rtvHandle.Offset(rtvDescriptorSize);
	}
}


ComPtr<ID3D12CommandAllocator> CreateCommandAllocator(ComPtr<ID3D12Device2> device, D3D12_COMMAND_LIST_TYPE type){
	ComPtr<ID3D12CommandAllocator> commandAllocator;
	ThrowIfFailed(device->CreateCommandAllocator(type, IID_PPV_ARGS(&commandAllocator)));

	return commandAllocator;
}


ComPtr<ID3D12GraphicsCommandList> CreateCommandList(ComPtr<ID3D12Device2> device, ComPtr<ID3D12CommandAllocator> commandAllocator, D3D12_COMMAND_LIST_TYPE type){
	ComPtr<ID3D12GraphicsCommandList> commandList;
	ThrowIfFailed(device->CreateCommandList(0, type, commandAllocator.Get(), nullptr, IID_PPV_ARGS(&commandList)));
	ThrowIfFailed(commandList->Close());
	return commandList;
}

ComPtr<ID3D12Fence> CreateFence(ComPtr<ID3D12Device2> device){
	ComPtr<ID3D12Fence> fence;
	ThrowIfFailed(device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&fence)));

	return fence;
}

HANDLE CreateEventHandle(){
	HANDLE fenceEvent;
	fenceEvent = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent&&"failed to create fence event");
	return fenceEvent;
}

uint64_t Signal(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t &fenceValue){
	uint64_t fenceValueForSignal = ++fenceValue;
	ThrowIfFailed(commandQueue->Signal(fence.Get(), fenceValueForSignal));
	return fenceValueForSignal;
}


void WaitForFenceValue(ComPtr<ID3D12Fence> fence, uint64_t fenceValue, HANDLE fenceEvent,
	std::chrono::milliseconds duration = std::chrono::milliseconds::max()){

	if(fence->GetCompletedValue()<fenceValue){
		ThrowIfFailed(fence->SetEventOnCompletion(fenceValue, fenceEvent));
		::WaitForSingleObject(fenceEvent, static_cast<DWORD>(duration.count()));
	}
}

void Flush(ComPtr<ID3D12CommandQueue> commandQueue, ComPtr<ID3D12Fence> fence, uint64_t &fenceValue, HANDLE fenceEvent){
	uint64_t fenceValueForSignal = Signal(commandQueue, fence, fenceValue);
	WaitForFenceValue(fence, fenceValueForSignal, fenceEvent);
}

void Update(){
	static	uint64_t frameCounter = 0;
	static	double elapsedSeconds = 0.0;
	static	std::chrono::high_resolution_clock clock;
	static	auto t0 = clock.now();

	frameCounter++;
	auto t1 = clock.now();
	auto deltaTime = t1-t0;
	t0 = t1;

	elapsedSeconds += deltaTime.count()*1e-9;
	if(elapsedSeconds>1.0){
		char buffer[500];
		auto fps = frameCounter/elapsedSeconds;
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		OutputDebugStringA(buffer);
		frameCounter = 0;
		elapsedSeconds = 0;

	}


}

void Render(){
	auto commandAllocator = g_CommandAllocators[g_CurrentBackBufferIndex];
	auto backBuffer = g_BackBuffers[g_CurrentBackBufferIndex];
	commandAllocator->Reset();
	g_CommandList->Reset(commandAllocator.Get(), nullptr);


	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	g_CommandList->ResourceBarrier(1, &barrier);

	FLOAT clearColor[] = {0.4f,0.6f,0.9f,1.0f};
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv(g_RTVDescriptorHeap->GetCPUDescriptorHandleForHeapStart(), g_CurrentBackBufferIndex, g_RTVDescriptorSize);
	g_CommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer.Get(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	g_CommandList->ResourceBarrier(1, &barrier);

	ThrowIfFailed(g_CommandList->Close());

	ID3D12CommandList *const commandLists[] = {
		g_CommandList.Get()
	};

	g_CommandQueue->ExecuteCommandLists(_countof(commandLists), commandLists);

	UINT syncInterval = g_Vsync ? 1 : 0;
	UINT presentFlags = 0;
	ThrowIfFailed(g_SwapChain->Present(syncInterval, presentFlags));
	g_FrameFenceValues[g_CurrentBackBufferIndex] = Signal(g_CommandQueue, g_Fence, g_FenceValue);

	g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
	WaitForFenceValue(g_Fence, g_FrameFenceValues[g_CurrentBackBufferIndex], g_FenceEvent);



}

void Resize(uint32_t width, uint32_t height){
	if(g_ClientWidth!=width||g_ClientHeight!=height){
		g_ClientWidth = std::max(1u, width);
		g_ClientHeight = std::max(1u, height);

		Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);

		for(int i = 0; i<g_NumFrames; ++i){
			g_BackBuffers[i].Reset();
			g_FrameFenceValues[i] = g_FrameFenceValues[g_CurrentBackBufferIndex];
		}

		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(g_SwapChain->GetDesc(&swapChainDesc));
		ThrowIfFailed(g_SwapChain->ResizeBuffers(g_NumFrames, g_ClientWidth, g_ClientHeight, swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
		g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
		UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);

	}
}

void SetFullscreen(bool fullScreen){
	if(g_Fullscreen!=fullScreen){
		g_Fullscreen = fullScreen;
		if(g_Fullscreen){
			::GetWindowRect(g_hwnd, &g_windowRect);
			UINT windowStyle = WS_OVERLAPPEDWINDOW&~(WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
			::SetWindowLongW(g_hwnd, GWL_STYLE, windowStyle);
			HMONITOR hMonitor = ::MonitorFromWindow(g_hwnd, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfo(hMonitor, &monitorInfo);
			::SetWindowPos(g_hwnd, HWND_TOP, monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.top, monitorInfo.rcMonitor.right-monitorInfo.rcMonitor.left, monitorInfo.rcMonitor.bottom-monitorInfo.rcMonitor.top, SWP_FRAMECHANGED|SWP_NOACTIVATE);
			::ShowWindow(g_hwnd, SW_MAXIMIZE);
		}
		else{
			::SetWindowLong(g_hwnd, GWL_STYLE, WS_OVERLAPPEDWINDOW);
			::SetWindowPos(g_hwnd, HWND_NOTOPMOST, g_windowRect.left, g_windowRect.top, g_windowRect.right-g_windowRect.left, g_windowRect.bottom-g_windowRect.top, SWP_FRAMECHANGED|SWP_NOACTIVATE);
			::ShowWindow(g_hwnd, SW_NORMAL);
		}
	}
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int nCmdShow){
	const wchar_t Class_Name[] = L"Render Engine";
	ParseCommandLineArguments();
	EnableDebugLayer();
	RegisterWindowClass(hInstance, Class_Name);
	HWND g_hwnd = CreateWindow(Class_Name, hInstance, L"Hello Triangle", g_ClientWidth, g_ClientHeight);
	::GetWindowRect(g_hwnd, &g_windowRect);
	ComPtr<IDXGIAdapter4> dxgiAdapter4 = Getadapter(g_UseWarp);
	g_Device = CreateDevice(dxgiAdapter4);
	g_CommandQueue = CreateCommandQueue(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);
	g_SwapChain = CreateSwapChain(g_hwnd, g_CommandQueue, g_ClientWidth, g_ClientHeight, g_NumFrames);
	g_CurrentBackBufferIndex = g_SwapChain->GetCurrentBackBufferIndex();
	g_RTVDescriptorHeap = CreateDescriptorHeap(g_Device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, g_NumFrames);
	g_RTVDescriptorSize = g_Device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	UpdateRenderTargetViews(g_Device, g_SwapChain, g_RTVDescriptorHeap);
	for(int i = 0; i<g_NumFrames; ++i){
		g_CommandAllocators[i] = CreateCommandAllocator(g_Device, D3D12_COMMAND_LIST_TYPE_DIRECT);

	}
	g_CommandList = CreateCommandList(g_Device, g_CommandAllocators[g_CurrentBackBufferIndex], D3D12_COMMAND_LIST_TYPE_DIRECT);
	g_Fence = CreateFence(g_Device);
	g_FenceEvent = CreateEventHandle();

	g_IsInitialised = true;

	::ShowWindow(g_hwnd, SW_SHOW);

	MSG msg = {};
	while(::GetMessage(&msg, NULL, 0, 0)){
		::TranslateMessage(&msg);
		::DispatchMessage(&msg);
	}
	Flush(g_CommandQueue, g_Fence, g_FenceValue, g_FenceEvent);
	::CloseHandle(g_FenceEvent);
	return 0;
}

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam){
	if(g_IsInitialised){
		switch(uMsg){
		case WM_SIZE: {
			RECT clientRect = {};
			::GetClientRect(g_hwnd, &clientRect);
			int width = clientRect.right-clientRect.left;
			int height = clientRect.bottom-clientRect.top;

			Resize(width, height);

		}
					break;
		case WM_DESTROY: {
			::PostQuitMessage(0);
			return 0;
		}

					   break;
		case WM_PAINT: {
			Update();
			Render();
		}
					 break;
		default: {
			return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
		}
		}
	}
	else{

		return ::DefWindowProc(hwnd, uMsg, wParam, lParam);
	}
}
