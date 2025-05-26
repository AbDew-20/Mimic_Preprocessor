#pragma once

#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>

#include <d3d12.h>
#include <dxgi1_6.h>

#include <string>

class Window{
public:
	static const UINT  kBufferCount = 2;

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

protected:

	friend LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);

	friend class Application;
	friend class Game;

	Window() = delete;
	Window(HWND hwnd, const std::wstring &windowName, int clientwidth, int clientheight, bool vSync);

	virtual ~Window();

	void RegisterCallbacks(Game *pGame);

	virtual void OnUpdate(double deltaTime, double totalTime);
	virtual void OnRender(double deltaTime, double totalTime);
	virtual void OnResize(int clientHeight, int clientWidth);

	IDXGISwapChain4 *CreateSwapChain();
	void UpdateRenderTargetViews();

private:
	Window(const Window &window) = delete;
	Window &operator =(const Window &other) = delete;
	HWND m_hWnd;
	std::wstring m_windowName;
	int m_clientWidth;
	int m_clientheight;
	bool m_vSync;
	bool m_fullscreen;

	uint64_t m_frameCounter;

	Game *m_pGame;
	IDXGISwapChain4 *m_dxgiSwapChain;
	ID3D12DescriptorHeap *m_d3d12RTVDescriptorHeap;
	ID3D12Resource *m_d3d12BackBuffers[kBufferCount];

	UINT m_RTVDescriptorSize;
	UINT m_currentBackBufferIndex;

	RECT m_windowRect;
	bool m_isTearingSupported;

};

