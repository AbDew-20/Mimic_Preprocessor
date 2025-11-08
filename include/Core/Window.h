#pragma once
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <dxgi1_6.h>
#include <string>
#include <Core/Helper.h>
#include <Core/Clock.h>
class Application;
class Window{
public:
	static const UINT kBufferCount = 3;
	Window(Application *app, HWND hwnd, const std::wstring &windowName, int clientwidth, int clientheight, bool vSync);
	Window() = delete;
	Window(Window &&window) = default;
	Window &operator = (Window &&other) noexcept;
	HWND GetWindowHandle() const;
	void Destroy();
	const std::wstring &GetWindowName() const;
	int GetClientHeight() const;
	int GetClientWidth() const;
	bool IsVSync() const;
	void SetVSync(bool vSync);
	void ToggleVSync();
	bool IsFullscreen() const;
	void SetFullscreen(bool fullscreen);
	void ToggleFullscreen();
	void Show();
	void Hide();
	UINT GetCurrentBackbufferIndex() const;
	ID3D12Resource *GetCurrentBackBuffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE GetCurrentRenderTargetView() const;
	UINT Present();
	void Init();
protected:
	friend LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
	friend class Application;
	friend class Game;
	void RegisterCallbacks(Game *pGame);
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnResize(int clientHeight, int clientWidth);
	inline uint64_t GetFrameCounter(){ return frameCounter_; }
	IDXGISwapChain4 *CreateSwapChain();
	void UpdateRenderTargetViews();
private:
	HWND hWnd_;
	std::wstring windowName_;
	int clientWidth_;
	int clientHeight_;
	uint64_t frameCounter_;
	Game *pGame_;
	IDXGISwapChain4 *pDxgiSwapChain_;
	ID3D12DescriptorHeap *pD3d12RTVDescriptorHeap_;
	ID3D12Resource *pD3d12BackBuffers_[kBufferCount];
	UINT rtvDescriptorSize_;
	UINT currentBackBufferIndex_;
	RECT windowRect_;
	bool isTearingSupported_;
	bool vSync_;
	bool fullscreen_;
	Application *app_;
	Clock updateClock_;
	Clock renderClock_;
};
