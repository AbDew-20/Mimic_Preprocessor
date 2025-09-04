#include <Window.h>
#include <Application.h>
#include <PCH.h>
#include <Game.h>
Window::Window(Application *app, HWND hwnd, const std::wstring &windowName, int clientWidth, int clientHeight, bool vSync) :
	hWnd_(hwnd),
	windowName_(windowName),
	clientHeight_(clientHeight),
	clientWidth_(clientWidth),
	vSync_(vSync),
	fullscreen_(false),
	frameCounter_(0),
	app_(app),
	currentBackBufferIndex_ (0)
	{}
Window &Window::operator= (Window &&other) noexcept{
	hWnd_ = other.hWnd_;
	windowName_ = std::move(other.windowName_);
	clientHeight_ = other.clientHeight_;
	clientWidth_ = other.clientWidth_;
	vSync_ = other.vSync_;
	fullscreen_ = other.fullscreen_;
	frameCounter_ = other.frameCounter_;
	app_ = other.app_;
	return *this;

}

void Window::Init(){
	isTearingSupported_ = app_->IsTearingSupported();
	pDxgiSwapChain_ = CreateSwapChain();
	pD3d12RTVDescriptorHeap_ = app_->CreateDescriptorHeap(kBufferCount, D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	rtvDescriptorSize_ = app_->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);

	UpdateRenderTargetViews();
}
void Window::OnUpdate(double deltaTime, double totalTime){
	updateClock_.Tick();
	frameCounter_++;
	pGame_->OnUpdate(updateClock_.GetDeltaSeconds(), updateClock_.GetTotalSeconds());
}
void Window::OnRender(double deltaTime, double totalTime){
	renderClock_.Tick();
	pGame_->OnRender(renderClock_.GetDeltaSeconds(), renderClock_.GetTotalSeconds());
}
void Window::OnResize(int clientHeight, int clientWidth){
	if(clientHeight_!=clientHeight||clientHeight_!=clientHeight_){
		clientHeight_ = std::max(1, clientHeight);
		clientWidth_ = std::max(1, clientWidth);

		app_->Flush();

		for(int i = 0; i<kBufferCount; i++){
			SafeRelease(pD3d12BackBuffers_[i]);
		}
		DXGI_SWAP_CHAIN_DESC swapChainDesc = {};
		ThrowIfFailed(pDxgiSwapChain_->GetDesc(&swapChainDesc));
		ThrowIfFailed(pDxgiSwapChain_->ResizeBuffers(kBufferCount, clientWidth_, clientHeight_,
			swapChainDesc.BufferDesc.Format, swapChainDesc.Flags));
		currentBackBufferIndex_ = pDxgiSwapChain_->GetCurrentBackBufferIndex();
		UpdateRenderTargetViews();

		pGame_->OnResize(clientHeight, clientWidth);

	}

}
IDXGISwapChain4 *Window::CreateSwapChain(){

	IDXGISwapChain4 *pDxgiSwapChain4;
	IDXGIFactory4 *pDxgiFactory4;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&pDxgiFactory4)));
	DXGI_SWAP_CHAIN_DESC1 swapChainDesc = {};
	swapChainDesc.Width = clientWidth_;
	swapChainDesc.Height = clientHeight_;
	swapChainDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	swapChainDesc.Stereo = FALSE;
	swapChainDesc.SampleDesc = {1, 0};
	swapChainDesc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	swapChainDesc.BufferCount = kBufferCount;
	swapChainDesc.Scaling = DXGI_SCALING_STRETCH;
	swapChainDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	swapChainDesc.AlphaMode = DXGI_ALPHA_MODE_UNSPECIFIED;
	swapChainDesc.Flags = isTearingSupported_ ? DXGI_SWAP_CHAIN_FLAG_ALLOW_TEARING : 0;
	IDXGISwapChain1 *pSwapChain1;
	ID3D12CommandQueue *commandQueue = app_->GetCommandQueue()->GetCommandQueue();
	ThrowIfFailed(pDxgiFactory4->CreateSwapChainForHwnd(commandQueue, hWnd_, &swapChainDesc, nullptr, nullptr, &pSwapChain1)); 
	ThrowIfFailed(pDxgiFactory4->MakeWindowAssociation(hWnd_, DXGI_MWA_NO_ALT_ENTER));
	ThrowIfFailed(pSwapChain1->QueryInterface(IID_IDXGISwapChain4, reinterpret_cast<void **>(&pDxgiSwapChain4)));
	SafeRelease(pSwapChain1);
	SafeRelease(pDxgiFactory4);
	return pDxgiSwapChain4;



}


void Window::UpdateRenderTargetViews(){ 
	ID3D12Device2 *pDevice = app_->GetDevice();
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtvHandle(pD3d12RTVDescriptorHeap_->GetCPUDescriptorHandleForHeapStart());
	for(int i = 0; i<kBufferCount; i++){
		ID3D12Resource *pBackBuffer;
		ThrowIfFailed(pDxgiSwapChain_->GetBuffer(i, IID_PPV_ARGS(&pBackBuffer)));
		pDevice->CreateRenderTargetView(pBackBuffer, nullptr, rtvHandle);
		pD3d12BackBuffers_[i] = pBackBuffer;
		rtvHandle.Offset(rtvDescriptorSize_);
	}
}
D3D12_CPU_DESCRIPTOR_HANDLE Window::GetCurrentRenderTargetView() const{
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(pD3d12RTVDescriptorHeap_->GetCPUDescriptorHandleForHeapStart(), currentBackBufferIndex_, rtvDescriptorSize_);

}
UINT Window::GetCurrentBackbufferIndex() const{
	return currentBackBufferIndex_;
}
ID3D12Resource *Window::GetCurrentBackBuffer() const{
	return pD3d12BackBuffers_[currentBackBufferIndex_];
}

HWND Window::GetWindowHandle()const{
	return hWnd_;
}
const std::wstring &Window::GetWindowName()const{
	return windowName_;
}
void Window::Show(){
	::ShowWindow(hWnd_, SW_SHOW);
}
void Window::Hide(){
	::ShowWindow(hWnd_, SW_HIDE);
}
void Window::Destroy(){
	pGame_->OnWindowDestroy();
	if(hWnd_){
		::DestroyWindow(hWnd_);
		hWnd_ = nullptr;
		for(int i = 0; i<kBufferCount; i++){
			SafeRelease(pD3d12BackBuffers_[i]);
		}
		SafeRelease(pD3d12RTVDescriptorHeap_);
		SafeRelease(pDxgiSwapChain_);
		windowName_.erase();
	}
	return;
}
int Window::GetClientHeight()const{
	return clientHeight_;
}
int Window::GetClientWidth()const{
	return clientWidth_;
}
bool Window::IsVSync()const{
	return vSync_;
}
void Window::SetVSync(bool vSync){
	vSync_ = vSync;
}
void Window::ToggleVSync(){
	SetVSync(!vSync_);
}
bool Window::IsFullscreen()const{
	return fullscreen_;
}

void Window::SetFullscreen(bool fullscreen){
	if(fullscreen_!=fullscreen){
		fullscreen_ = fullscreen;
		if(fullscreen_){
			::GetWindowRect(hWnd_, &windowRect_);

			UINT windowStyle = WS_OVERLAPPED&~(WS_CAPTION|WS_SYSMENU|WS_THICKFRAME|WS_MINIMIZEBOX|WS_MAXIMIZEBOX);
			::SetWindowLongW(hWnd_, GWL_STYLE, windowStyle);

			HMONITOR hMonitor = ::MonitorFromWindow(hWnd_, MONITOR_DEFAULTTONEAREST);
			MONITORINFOEX monitorInfo = {};
			monitorInfo.cbSize = sizeof(MONITORINFOEX);
			::GetMonitorInfoW(hMonitor, &monitorInfo);

			::SetWindowPos(hWnd_, HWND_NOTOPMOST, windowRect_.left, windowRect_.top,
				windowRect_.right-windowRect_.left, windowRect_.top-windowRect_.bottom,
				SWP_FRAMECHANGED|SWP_NOACTIVATE);
			::ShowWindow(hWnd_, SW_MAXIMIZE);
		}
		else{
			::SetWindowLongW(hWnd_, GWL_STYLE, WS_OVERLAPPEDWINDOW);
			::SetWindowPos(hWnd_, HWND_NOTOPMOST, windowRect_.left, windowRect_.top,
				windowRect_.right-windowRect_.left, windowRect_.top-windowRect_.bottom,
				SWP_FRAMECHANGED|SWP_NOACTIVATE);
			::ShowWindow(hWnd_, SW_NORMAL);
		}
	}
}


void Window::ToggleFullscreen(){
	SetFullscreen(!fullscreen_);
}
void Window::RegisterCallbacks(Game *pGame){
	pGame_ = pGame;
}
UINT Window::Present(){
	UINT syncInterval = vSync_ ? 1 : 0;
	UINT presentFlags = isTearingSupported_&&!vSync_ ? DXGI_PRESENT_ALLOW_TEARING : 0;
	ThrowIfFailed(pDxgiSwapChain_->Present(syncInterval, presentFlags));
	currentBackBufferIndex_ = pDxgiSwapChain_->GetCurrentBackBufferIndex();

	return currentBackBufferIndex_;
}
