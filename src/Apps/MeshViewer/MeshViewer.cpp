#include <Apps/MeshViewer/BaseContext.h>
#include <Apps/MeshViewer/InputContext.h>
#include <Apps/MeshViewer/MeshViewer.h>
#include <Apps/MeshViewer/SplashContext.h>
#include <Apps/MeshViewer/GraphContext.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <Core/Application.h>
#include <Core/CommandQueue.h>
#include <Core/PCH.h>
#include <imgui.h>
#include <implot.h>




MeshViewer::MeshViewer(Application *pApp, const std::wstring &name, int width, int height, bool vSync) :
	super(pApp, name, width, height, vSync),
	pApp_(pApp),
	pDepthBuffer_(nullptr),
	pDsvHeap_(nullptr),
	imguiSRVAlloc_(64, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pApp)
{

}


bool MeshViewer::LoadContent(){
	contextStack_.reserve(2);
	currentState_ = SplashState{};
	contextStack_.push_back(SplashContext());
	ID3D12Device2 *pDevice = pApp_->GetDevice(); 


	pDsvHeap_ = pApp_->CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	CreateDepthBuffer(GetClientWidth(), GetClientHeight()); 
	
	InitImgui();
	std::string directory = CONFIG_PATH;
	std::string fileName = "contexts.txt";
	InputManager *pInputManager = pApp_->GetInputManager();
	pInputManager->LoadContexts(fileName, directory, InputContext::GetActionId, InputContext::GetStateId);
	std::string context = "MeshContext";
	pInputManager->PushContext(context);
	pInputManager->AddCallback([this](MappedInput &input){return this->HandleInput(input);}, 0);
	return true;
}

void MeshViewer::UnloadContent(){
	ContextCleanup();
	DestroyImgui();
	SafeRelease(pDepthBuffer_);
	SafeRelease(pDsvHeap_);
	imguiSRVAlloc_.Destroy();
}

void MeshViewer::TransitionState(){
	std::visit(
		[this](auto &args)->void{
			using T = std::decay_t<decltype(args)>;

			if constexpr(std::is_same_v<T, SplashState>){
				if(args.fileSelected){
					contextStack_.pop_back();
					contextStack_.push_back(ViewerContext(this->pApp_, args.filePath, this->asyncThread_));
					currentState_ = ViewerState{ViewerState::Mode::NORMAL,false, false, ""};
					this->GetContext<ViewerContext>()->Load();
					InputManager *pInputmanager = pApp_->GetInputManager();
					std::string contextName = "ViewerContext";
					pInputmanager->PushContext(contextName);
					pInputmanager->AddCallback([this](MappedInput &mappedInput){this->GetContext<ViewerContext>()->HandleInput(mappedInput);}, 1);
				}
			}
			else if constexpr(std::is_same_v<T, ViewerState>){
				InputManager *pInputmanager = pApp_->GetInputManager();
				if(args.asyncStarted){
					args.asyncStarted = false;
					args.mode = ViewerState::Mode::LOADING;
					contextStack_.push_back(LoadingContext(args.workType));
					std::string contextName = "LoadingContext";
					pInputmanager->PushContext(contextName);
				}
				if(args.asyncFinished){
					asyncThread_.Join();
					asyncThread_.Reset();
					args.asyncFinished = false;
					args.mode = ViewerState::Mode::NORMAL;
					contextStack_.pop_back();
					pInputmanager->PopContext();
				}
				if(args.loadGraph){
					args.loadGraph = false;
					DataAnalysis::DataAnalyzer &analyzer =this->GetContext<ViewerContext>()->GetAnalyzer();
					contextStack_.push_back(GraphContext(analyzer));
					previousState_ = currentState_;
					currentState_ = GraphState{GraphState::Mode::NORMAL, false, false,""};
				}
			}
			else if constexpr(std::is_same_v<T, GraphState>){
				if(args.focusViewer){
					args.focusViewer = false;
					contextStack_.pop_back();
					currentState_ = previousState_;
				}
				InputManager *pInputmanager = pApp_->GetInputManager();
				if(args.asyncStarted){
					args.asyncStarted = false;
					args.mode = GraphState::Mode::LOADING;
					contextStack_.push_back(LoadingContext(args.workType));
					std::string contextName = "LoadingContext";
					pInputmanager->PushContext(contextName);
				}
				if(args.asyncFinished){
					asyncThread_.Join();
					asyncThread_.Reset();
					args.asyncFinished = false;
					args.mode = GraphState::Mode::NORMAL;
					contextStack_.pop_back();
					pInputmanager->PopContext();
				}
			
			}
			else{
				static_assert(false, "Variant not handled in visitor");
			}
		
		},
		currentState_
	);
}

void MeshViewer::OnResize(int height, int width){
	super::OnResize(height, width);
	CreateDepthBuffer(width, height);
}

void MeshViewer::OnKeyPress(KeyCodes key, bool shift, bool ctl, bool alt){
	switch(key){
	case KeyCodes::Esc:
		pApp_->Quit(0);
		break;
	}
}
void MeshViewer::OnKeyRelease(KeyCodes key, bool shift, bool ctl, bool alt){
}

void MeshViewer::HandleInput(MappedInput &input){
	using namespace InputContext;
	for(auto iter = input.Actions.begin(); iter!=input.Actions.end(); ++iter){
		switch(static_cast<Actions>(*iter)){
		case Actions::ToggleFullscren:
			pWindow->ToggleFullscreen();
			input.ConsumeAction((size_t)Actions::ToggleFullscren);
			break;
		case Actions::ToggleVsync:
			pWindow->ToggleVSync();
			input.ConsumeAction((size_t)Actions::ToggleVsync);
			break;
		}

		if(input.Actions.begin()==input.Actions.end()){
			break;
		}
	
	}
}


void MeshViewer::OnUpdate(double deltaTime, double totalTime){
	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
	std::visit(
		[this, deltaTime, totalTime](auto &args){
			using T = std::decay_t<decltype(args)>;
			if constexpr(std::is_same_v<T, SplashState>){
				SplashContext *pSplashContext = GetContext<SplashContext>();
				assert(pSplashContext !=nullptr&&"Context Missing");
				SplashStateParams splashParams = {this->GetClientWidth(), this->GetClientHeight(), args.fileSelected, args.filePath};
				pSplashContext->Update(splashParams, deltaTime);
				
			}
			else if constexpr(std::is_same_v<T, ViewerState>){
				ViewerContext *pViewerContext = GetContext<ViewerContext>();
				assert(pViewerContext!=nullptr && "Context Missing");

				ViewerStateParams viewerParams = {this->GetClientWidth(), this->GetClientHeight(), (args.mode==ViewerState::Mode::LOADING), args.asyncStarted, args.workType, args.loadGraph};
				pViewerContext->Update(viewerParams, deltaTime, totalTime);
				if(args.mode==ViewerState::Mode::LOADING){
					LoadingContext *pLoadingContext = GetContext<LoadingContext>();
					assert(pLoadingContext!=nullptr&&"Context Missing");

					LoadingStateParams loadingParams = {this->GetClientWidth(), this->GetClientHeight(), this->asyncThread_.GetPercent(), args.workType};
					pLoadingContext->Update(loadingParams, deltaTime);
					args.asyncFinished = this->asyncThread_.GetCompleted();
				}
			
			}
			else if constexpr(std::is_same_v<T, GraphState>){
				GraphContext *pGraphContext = GetContext<GraphContext>();
				assert(pGraphContext!=nullptr&&"ContextMissing");

				GraphStateParams graphParams = {this->GetClientWidth(), this->GetClientHeight(), args.focusViewer};
				pGraphContext->Update(graphParams, deltaTime);
				if(args.mode==GraphState::Mode::LOADING){
					LoadingContext *pLoadingContext = GetContext<LoadingContext>();
					assert(pLoadingContext!=nullptr&&"Context Missing");

					LoadingStateParams loadingParams = {this->GetClientWidth(), this->GetClientHeight(), this->asyncThread_.GetPercent(), args.workType};
					pLoadingContext->Update(loadingParams, deltaTime);
					args.asyncFinished = this->asyncThread_.GetCompleted();
				}
			
			}
			else{
				static_assert(false, "Variant not handled in visitor");
			}
		}
		,currentState_);
}

void MeshViewer::OnRender(double deltaTime, double totalTime){
	ImGui::Render();
	ID3D12Resource* pBackBuffer = pWindow->GetCurrentBackBuffer();
	UINT currentBackBufferIdx = pWindow->GetCurrentBackbufferIndex();
	ID3D12GraphicsCommandList4* pCommandList = pApp_->GetCommandQueue()->GetCommandList();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = pDsvHeap_->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = pWindow->GetCurrentRenderTargetView();

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pCommandList->ResourceBarrier(1, &barrier);

	D3D12_VIEWPORT viewPort = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(GetClientWidth()), static_cast<float>(GetClientHeight()));
	pCommandList->RSSetViewports(1,&viewPort);

	D3D12_RECT scissorRect = CD3DX12_RECT(0,0, LONG_MAX, LONG_MAX);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	for(Context &context:contextStack_){
		std::visit(
			[rtv, dsv, pCommandList, deltaTime](auto &args){
				args.Render(rtv, dsv, pCommandList, deltaTime);
			}, context);
	}

	pCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);
	pCommandList->SetDescriptorHeaps(1, imguiSRVAlloc_.GetHeapPointerLocation(0));
	ImGui_ImplDX12_RenderDrawData(ImGui::GetDrawData(), pCommandList);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(pBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	pCommandList->ResourceBarrier(1, &barrier);
	fenceValues_[currentBackBufferIdx] = pApp_->GetCommandQueue()->ExecuteCommandList(pCommandList);
		
	currentBackBufferIdx = pWindow->Present();

	pApp_->GetCommandQueue()->WaitForFenceValue(fenceValues_[currentBackBufferIdx]);

}

void MeshViewer::OnWindowDestroy(){
	pWindow = nullptr;
}

void MeshViewer::ContextCleanup(){
	for(Context context : contextStack_){
		std::visit([](auto &&args){
			using T = std::decay_t<decltype(args)>;
				if constexpr(std::is_same_v<T, ViewerContext>){
					args.Unload();
				}
			}, context);
	
	}
}

void MeshViewer::CreateDepthBuffer(int width, int height){
	pApp_->Flush();
	height = std::max(1, height);
	width = std::max(1, width);

	ID3D12Device2 *pDevice = pApp_->GetDevice();

	D3D12_CLEAR_VALUE clearValue = {};
	clearValue.Format = DXGI_FORMAT_D32_FLOAT;
	clearValue.DepthStencil = {1.0f, 0};
	SafeRelease(pDepthBuffer_);

	D3D12_HEAP_PROPERTIES heapProp =::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC resourceDesc = ::CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1, 0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL);
	ThrowIfFailed(pDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_DEPTH_WRITE,
		&clearValue,
		IID_ID3D12Resource,
		reinterpret_cast<void **>(&pDepthBuffer_)));
	
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv = {};
	dsv.Format = DXGI_FORMAT_D32_FLOAT;
	dsv.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv.Texture2D.MipSlice = 0;
	dsv.Flags = D3D12_DSV_FLAG_NONE;

	pDevice->CreateDepthStencilView(
		pDepthBuffer_,
		&dsv,
		pDsvHeap_->GetCPUDescriptorHandleForHeapStart());


}

void MeshViewer::InitImgui(){
	ImGui_ImplWin32_EnableDpiAwareness();
	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImPlot::CreateContext();

	ImGuiIO &io = ImGui::GetIO(); 
	(void)io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     	
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;

	ImGui::StyleColorsDark();

	// Setup scaling
	ImGuiStyle &style = ImGui::GetStyle();
	style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
	style.FontScaleDpi = main_scale;        // Set initial font scale. (using io.ConfigDpiScaleFonts=true makes this unnecessary. We leave both here for documentation purpose)

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(pWindow->GetWindowHandle());

	ImGui_ImplDX12_InitInfo init_info = {};
	init_info.Device = pApp_->GetDevice();
	init_info.CommandQueue = pApp_->GetCommandQueue()->GetCommandQueue();
	init_info.NumFramesInFlight = pWindow->GetMaxBufferCount();
	init_info.RTVFormat = DXGI_FORMAT_R8G8B8A8_UNORM;
	init_info.DSVFormat = DXGI_FORMAT_UNKNOWN;
	init_info.UserData = this;
	// Allocating SRV descriptors (for textures) is up to the application, so we provide callbacks.
	// (current version of the backend will only allocate one descriptor, future versions will need to allocate more)
	init_info.SrvDescriptorHeap = imguiSRVAlloc_.GetHeapPointer(0);
	init_info.SrvDescriptorAllocFn = [](ImGui_ImplDX12_InitInfo *info, D3D12_CPU_DESCRIPTOR_HANDLE *out_cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE *out_gpu_handle) {
		auto self = static_cast<MeshViewer *>(info->UserData);
		return self->imguiSRVAlloc_.Alloc(out_cpu_handle, out_gpu_handle); };
	init_info.SrvDescriptorFreeFn = [](ImGui_ImplDX12_InitInfo *info, D3D12_CPU_DESCRIPTOR_HANDLE cpu_handle, D3D12_GPU_DESCRIPTOR_HANDLE gpu_handle) {
		auto self = static_cast<MeshViewer *>(info->UserData);
		return self->imguiSRVAlloc_.Free(cpu_handle, gpu_handle); };
	ImGui_ImplDX12_Init(&init_info);
}

void MeshViewer::DestroyImgui(){
	ImGui_ImplDX12_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImPlot::DestroyContext();
	ImGui::DestroyContext();
}

