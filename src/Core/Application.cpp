#include <Core/Application.h>
#include <Core/PCH.h>
#include <Core/Game.h>
constexpr wchar_t kWindowClassName[] = L"Mimic_Engine";
static LRESULT CALLBACK WndProc(HWND hwnd, UINT message, WPARAM wParam, LPARAM lParam);
void Application::EnableDebugLayers(){
#if defined(_DEBUG)
	ID3D12Debug *pDebugInterface;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(&pDebugInterface)));
	pDebugInterface->EnableDebugLayer();
	pDebugInterface_ = pDebugInterface;
#endif
}
Application::Application(HINSTANCE hInstance) :
	hInstance_(hInstance),
	tearingSupported_(false),
	windowPool_(GetNumMaxWindow()),
	commandQueuePool_(3){}


void Application::Init(){
	EnableDebugLayers();
	WNDCLASSEXW windClass = {};
	windClass.cbSize = sizeof(WNDCLASSEXW);
	windClass.style = CS_HREDRAW||CS_VREDRAW;
	windClass.lpfnWndProc = &WndProc;
	windClass.hInstance = hInstance_;
	windClass.lpszClassName = kWindowClassName;
	windClass.lpszMenuName = NULL;
	windClass.hbrBackground = (HBRUSH)(COLOR_WINDOW+1);
	if(!::RegisterClassExW(&windClass)){
		::MessageBox(NULL, "Unable to register window class", "Error", MB_OK|MB_ICONERROR);
	}
	pDxgiAdapter_ = Application::GetAdapter(false);
	if(pDxgiAdapter_){
		pDevice_ = Application::CreateDevice(pDxgiAdapter_);
	}
	if(pDevice_){
		pDirectCommandQueue_ = commandQueuePool_.Emplace(pDevice_, D3D12_COMMAND_LIST_TYPE_DIRECT);
		pDirectCommandQueue_->Init();
		pComputeCommandQueue_ = commandQueuePool_.Emplace(pDevice_, D3D12_COMMAND_LIST_TYPE_COMPUTE);
		pComputeCommandQueue_->Init();
		pCopyCommandQueue_ = commandQueuePool_.Emplace(pDevice_, D3D12_COMMAND_LIST_TYPE_COPY);
		pCopyCommandQueue_->Init();
		
		tearingSupported_ = CheckTearingSupport();
	}
}


void Application::ShutDown(){
	Flush();
	pDirectCommandQueue_->Destroy();
	pDirectCommandQueue_ = nullptr;
	pComputeCommandQueue_->Destroy();
	pComputeCommandQueue_ = nullptr;
	pCopyCommandQueue_->Destroy();
	pCopyCommandQueue_ = nullptr;
	SafeRelease(pDevice_);
	SafeRelease(pDxgiAdapter_);
#if defined(_DEBUG)
	SafeRelease(pDebugInterface_);
#endif
}
Application::~Application(){
	bool empty= windowMap_.empty();
	assert(windowMap_.empty()&&"Windows not destroyed");
}
IDXGIAdapter4 *Application::GetAdapter(bool useWarp){
	IDXGIFactory4 *pDxgiFactory;
	UINT createFactoryFlags = 0;
#if defined(_DEBUG)
	createFactoryFlags = DXGI_CREATE_FACTORY_DEBUG;
#endif
	ThrowIfFailed(CreateDXGIFactory2(createFactoryFlags, IID_PPV_ARGS(&pDxgiFactory)));
	IDXGIAdapter1 *pDxgiAdapter1=nullptr;
	IDXGIAdapter4 *pDxgiAdapter4=nullptr;
	if(useWarp){
		ThrowIfFailed(pDxgiFactory->EnumWarpAdapter(IID_PPV_ARGS(&pDxgiAdapter1)));
		ThrowIfFailed(pDxgiAdapter1->QueryInterface(__uuidof(IDXGIAdapter4), reinterpret_cast<void **>(&pDxgiAdapter4)));
	}
	else{
		SIZE_T maxDedicatedMemory = 0;
		for(UINT i = 0; pDxgiFactory->EnumAdapters1(i, &pDxgiAdapter1)!=DXGI_ERROR_NOT_FOUND; ++i){
			DXGI_ADAPTER_DESC1 dxgiAdapterDesc1;
			pDxgiAdapter1->GetDesc1(&dxgiAdapterDesc1);
			if((dxgiAdapterDesc1.Flags&DXGI_ADAPTER_FLAG_SOFTWARE)==0&&SUCCEEDED(D3D12CreateDevice(pDxgiAdapter1, D3D_FEATURE_LEVEL_11_0, __uuidof(ID3D12Device), nullptr))&&dxgiAdapterDesc1.DedicatedVideoMemory>maxDedicatedMemory){
				SafeRelease(pDxgiAdapter4);
				maxDedicatedMemory = dxgiAdapterDesc1.DedicatedVideoMemory;
				ThrowIfFailed(pDxgiAdapter1->QueryInterface(__uuidof(IDXGIAdapter4), reinterpret_cast<void **>(&pDxgiAdapter4)));
			}
			SafeRelease(pDxgiAdapter1);
		}
	}
	SafeRelease(pDxgiFactory);


	return pDxgiAdapter4;
}
ID3D12Device2 *Application::CreateDevice(IDXGIAdapter4 *pAdapter){
	ID3D12Device2 *pD3d12Device2;
	ThrowIfFailed(D3D12CreateDevice(pAdapter, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(&pD3d12Device2)));
#if defined(_DEBUG)
	ID3D12InfoQueue *pInfoQueue;
	if(SUCCEEDED(pD3d12Device2->QueryInterface(IID_ID3D12InfoQueue, reinterpret_cast<void **>(&pInfoQueue)))){
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
	SafeRelease(pInfoQueue);
#endif
	return pD3d12Device2;
}

bool Application::CheckTearingSupport(){
	BOOL allowTearing = FALSE;
	IDXGIFactory4 *pDxgiFactory4;
	if(SUCCEEDED(CreateDXGIFactory1(IID_IDXGIFactory4, reinterpret_cast<void **>(&pDxgiFactory4)))){
		IDXGIFactory5 *pDxgiFactory5;
		if(SUCCEEDED(pDxgiFactory4->QueryInterface(IID_IDXGIFactory5, reinterpret_cast<void **>(&pDxgiFactory5)))){
			pDxgiFactory5->CheckFeatureSupport(DXGI_FEATURE_PRESENT_ALLOW_TEARING, &allowTearing, sizeof(allowTearing));
		}
		SafeRelease(pDxgiFactory5);
	}
	SafeRelease(pDxgiFactory4);
	return allowTearing==TRUE;
}
bool Application::IsTearingSupported()const{
	return tearingSupported_;
}
Window *Application::CreateRenderWindow(std::wstring &windowName, int clientWidth, int clientHeight, bool vSync){
	std::map<std::wstring, Window *>::iterator windowIter = windowNameMap_.find(windowName);
	if(windowIter!=windowNameMap_.end()){
		return windowIter->second;
	}
	RECT windowRect = {0,0, clientWidth,clientHeight};
	::AdjustWindowRect(&windowRect, WS_OVERLAPPEDWINDOW, FALSE);
	HWND hwnd = CreateWindowW(kWindowClassName, windowName.c_str()
		, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT
		, windowRect.right-windowRect.left, windowRect.bottom-windowRect.top
		, nullptr, nullptr, hInstance_, this);
	if(!hwnd){
		::MessageBox(NULL, "Could not create render window", "Error", MB_ICONERROR|MB_OK);
		return nullptr;
	}
	Window *pWindow = windowPool_.Emplace(this, hwnd, windowName, clientWidth, clientHeight, vSync);
	pWindow->Init();
	windowMap_.insert(std::map<HWND, Window *>::value_type(hwnd, pWindow));
	windowNameMap_.insert(std::map<std::wstring, Window *>::value_type(windowName, pWindow));
	return pWindow;
}

void Application::DestroyWindow(Window *pWindow){
	if(pWindow){
		Flush();
		auto it = windowMap_.find(pWindow->GetWindowHandle());
		windowMap_.erase(it);
		auto iter = windowNameMap_.find(pWindow->GetWindowName());
		windowNameMap_.erase(iter);
		pWindow->Destroy();
	}
	windowPool_.Remove(pWindow);
}

void Application::DestroyWindow(std::wstring &windowName){
	Window *pWindow = GetWindowByName(windowName);
	DestroyWindow(pWindow);
}

Window *Application::GetWindowByName(std::wstring &windowName){
	Window *pWindow=nullptr;
	std::map<std::wstring, Window * >::iterator iter = windowNameMap_.find(windowName);
	if(iter!=windowNameMap_.end()){
		pWindow = iter->second;
	}
	return pWindow;
}

ID3D12Device2 *Application::GetDevice() const{
	return pDevice_;
}

CommandQueue *Application::GetCommandQueue(D3D12_COMMAND_LIST_TYPE type)const{
	CommandQueue *pCommandQueue;
	switch(type){
	case D3D12_COMMAND_LIST_TYPE_DIRECT:
		pCommandQueue = pDirectCommandQueue_;
		break;
	case D3D12_COMMAND_LIST_TYPE_COMPUTE:
		pCommandQueue = pComputeCommandQueue_;
		break;
	case D3D12_COMMAND_LIST_TYPE_COPY:
		pCommandQueue = pCopyCommandQueue_;
		break;
	default:
		assert(false&&"Invalid command queue type");
	}
	return pCommandQueue;
}

void Application::Flush(){
	pDirectCommandQueue_->Flush();
	pCopyCommandQueue_->Flush();
	pComputeCommandQueue_->Flush();
}

ID3D12DescriptorHeap *Application::CreateDescriptorHeap(UINT numDescriptor, D3D12_DESCRIPTOR_HEAP_TYPE type){
	D3D12_DESCRIPTOR_HEAP_DESC desc = {};
	desc.Type = type;
	desc.NodeMask = 0;
	desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	desc.NumDescriptors = numDescriptor;
	ID3D12DescriptorHeap *pDescriptorHeap;
	ThrowIfFailed(pDevice_->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&pDescriptorHeap)));
	return pDescriptorHeap;

}
UINT Application::GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE type) const{
	return pDevice_->GetDescriptorHandleIncrementSize(type);

}
void Application::RemoveWindow(HWND hWnd){
	std::map<HWND, Window *>::iterator iter = windowMap_.find(hWnd);
	if(iter!=windowMap_.end()){
		Window *pWindow = iter->second;
		windowNameMap_.erase(pWindow->GetWindowName());
		windowMap_.erase(iter);
	}
}
void Application::Quit(int exitCode){
	PostQuitMessage(exitCode);
}
Window *Application::GetWindow(HWND hWnd){
	std::map<HWND, Window *>::iterator iter = windowMap_.find(hWnd);
	if(iter!=windowMap_.end()){
		return iter->second;
	}
	else{
		return nullptr;
	}
}

int Application::Run(Game *pGame){
	//TODO: Add Game init checks
	pGame->Initialize();
	pGame->LoadContent();


	bool running = true;
	MSG msg = {};
	while(running){
		while(::PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)){
			if(msg.message==WM_QUIT){
				running = false;
			}

			::TranslateMessage(&msg);
			::DispatchMessageW(&msg);
		
		}

		if(running){
			for(auto iter = windowMap_.begin(); iter!=windowMap_.end(); ++iter){
				iter->second->OnUpdate();
				iter->second->OnRender();
			}
				
		}
	
	}
	Flush();
	pGame->UnloadContent();
	//TODO: Add Game Cleanup

	return 0;
}


static LRESULT CALLBACK WndProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam){
	Window *pWindow=nullptr;
	Application *pApp=nullptr;
	if(message==WM_NCCREATE){
		CREATESTRUCT *pCs = (CREATESTRUCT *)lParam;
		SetWindowLongPtrW(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pCs->lpCreateParams));
		return TRUE;
	}else{
		pApp =reinterpret_cast<Application *>(GetWindowLongPtrW(hWnd, GWLP_USERDATA));
	}

	if(pApp){
		pWindow = pApp->GetWindow(hWnd);
	}

	if(pWindow){
		switch(message){
		case WM_PAINT:
		{
			::ValidateRect(hWnd,nullptr);
		}
		break;
		case WM_SIZE:
		{
			int width = ((int)(short)LOWORD(lParam));
			int height = ((int)(short)HIWORD(lParam));
			pWindow->OnResize(height, width);
		}
		break;
		case WM_CLOSE:
		{
			pApp->DestroyWindow(pWindow);
			
			if(pApp->IsWindowMapEmpty()){
				pApp->Quit(0);
			}
		}
		break;
		default:
			return DefWindowProcW(hWnd, message, wParam, lParam);

		}
	}
	else{
		return DefWindowProcW(hWnd, message, wParam, lParam);
	}

	return 0;

}
