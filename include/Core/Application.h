#pragma once
#include <directx/d3dx12.h>
#include <dxgi1_6.h>
#include <wrl.h>
#include <string>
#include <map>
#include <Utils/PoolAllocator.h>
#include <Core/Window.h>
#include <Core/CommandQueue.h>
#include <Core/InputManager.h>
class Game;
class CommandQueue;
class Application{
public:
	Application(HINSTANCE hInst);
	~Application();
	void Init();
	void ShutDown();
	bool IsTearingSupported() const;
	Window *CreateRenderWindow(std::wstring &windowName, int clientWidth, int clientHeight, bool vSync = true);
	void DestroyWindow(Window *pWindow);
	void DestroyWindow(std::wstring &windowName);
	Window *GetWindowByName(std::wstring &windowName);
	Window *GetWindow(HWND hWnd);
	inline bool IsWindowMapEmpty(){ return windowMap_.empty(); }
	int Run(Game *pGame);
	void Quit(int exitCode = 0);
	ID3D12Device2 *GetDevice() const;
	CommandQueue *GetCommandQueue(D3D12_COMMAND_LIST_TYPE = D3D12_COMMAND_LIST_TYPE_DIRECT) const;
	InputManager *GetInputManager(){ return &inputManager_; }
	void Flush();
	ID3D12DescriptorHeap *CreateDescriptorHeap(UINT numDescriptors, D3D12_DESCRIPTOR_HEAP_TYPE type, D3D12_DESCRIPTOR_HEAP_FLAGS flags);
	UINT GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const;
	inline std::map<HWND, Window *> *GetWindowMapRef(){ return &windowMap_; }
private:
	void RemoveWindow(HWND hWnd);
	IDXGIAdapter4 *GetAdapter(bool useWarp);
	ID3D12Device2 *CreateDevice(IDXGIAdapter4 *pAdapter);
	bool CheckTearingSupport();
	void EnableDebugLayers();
	inline constexpr size_t GetNumMaxWindow(){
		return 	5;
	}
	std::map<HWND, Window *> windowMap_;
	std::map<std::wstring, Window *> windowNameMap_;
	PoolAllocator<Window> windowPool_;
	PoolAllocator<CommandQueue> commandQueuePool_;
	InputManager inputManager_;
	HINSTANCE hInstance_;
	IDXGIAdapter4 *pDxgiAdapter_;
	ID3D12Device2 *pDevice_;
	CommandQueue *pDirectCommandQueue_;
	CommandQueue *pComputeCommandQueue_;
	CommandQueue *pCopyCommandQueue_;
	bool tearingSupported_;
};