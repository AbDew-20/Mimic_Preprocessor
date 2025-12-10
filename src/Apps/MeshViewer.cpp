#include <Apps/MeshViewer.h>
#include <Core/Application.h>
#include <Core/PCH.h>
#include <Core/CommandQueue.h>
#include <Utils/ScopedTimer.h>
#include <Utils/MeshTools.h>
#include <imgui.h>
#include <backends/imgui_impl_dx12.h>
#include <backends/imgui_impl_win32.h>
#include <ShObjIdl.h>




MeshViewer::MeshViewer(Application *pApp, const std::wstring &name, int width, int height, bool vSync) :
	super(pApp, name, width, height, vSync),
	pApp_(pApp),
	pDepthBuffer_(nullptr),
	pDsvHeap_(nullptr),
	cameraPos_(DirectX::XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f)),
	cameraVelocity_(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)),
	boundingBoxVisible_(false),
	meshIdx(0),
	zoom_(1.0f),
	imguiSRVAlloc_(64, D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV, D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE, pApp),
	gameState_(GameStates::SPLASH),
	nextState_(GameStates::SPLASH)
{
	threadSpawned_ = false;
}


bool MeshViewer::LoadContent(){

	ID3D12Device2 *pDevice = pApp_->GetDevice(); 

	//UploadDebugPassResources(indexedBBVertexData, bbIndexData);


	pDsvHeap_ = pApp_->CreateDescriptorHeap(1, D3D12_DESCRIPTOR_HEAP_TYPE_DSV, D3D12_DESCRIPTOR_HEAP_FLAG_NONE);

	CreateMainPassPipelineState();
	CreateDebugPassPipelineState();
	CreateDepthBuffer(GetClientWidth(), GetClientHeight()); 
	
	InitImgui();

	return true;
}

void MeshViewer::UnloadContent(){
	DestroyImgui();
	SafeRelease(pDepthBuffer_);
	SafeRelease(pPipelineState_[0]);
	SafeRelease(pPipelineState_[1]);
	SafeRelease(pRootSignature_);
	SafeRelease(pDsvHeap_);
	SafeRelease(pIndexBuffer_[0]);
	SafeRelease(pVertexBuffer_[0]);
	//SafeRelease(pIndexBuffer_[1]);
	//SafeRelease(pVertexBuffer_[1]);
	imguiSRVAlloc_.Destroy();
}

void MeshViewer::OnResize(int height, int width){
	super::OnResize(height, width);
	CreateDepthBuffer(width, height);
}

void MeshViewer::OnKeyPress(KeyCodes key, bool shift, bool ctl, bool alt){
	switch(key){
	case KeyCodes::F:
		pWindow->ToggleFullscreen();
		break;
	case KeyCodes::V:
		pWindow->ToggleVSync();
		break;
	case KeyCodes::Esc:
		pApp_->Quit(0);
		break;
	case KeyCodes::W:
		cameraVelocity_.y =	+GetCameraSpeed();
		break;			
	case KeyCodes::S:
		cameraVelocity_.y =	-GetCameraSpeed();
		break;			
	case KeyCodes::A:
		cameraVelocity_.x =	-GetCameraSpeed();
		break;			
	case KeyCodes::D:
		cameraVelocity_.x =	+GetCameraSpeed();
		break;			
	case KeyCodes::Q:
		cameraVelocity_.z =	+GetCameraSpeed();
		break;			
	case KeyCodes::E:
		cameraVelocity_.z =	-GetCameraSpeed();
		break;
	case KeyCodes::B:
		boundingBoxVisible_ = !boundingBoxVisible_;
		break;
	case KeyCodes::Up:
		if(meshIdx==occluderRankingData_.size()-1){
			meshIdx = 0;
		}
		else{
			meshIdx++;
		}
		DebugPrint("Occluder Score: %f\n", occluderRankingData_.at(meshIdx).first);
		break;
	case KeyCodes::Down:
		if(meshIdx==0){
			meshIdx = occluderRankingData_.size()-1;
		}
		else{
			meshIdx--;
		}
		DebugPrint("Occluder Score: %f\n", occluderRankingData_.at(meshIdx).first);
		break;
	case KeyCodes::Z:
		zoom_ *= 0.5;
		break;
	case KeyCodes::X:
		zoom_ *= 2;
	
	}

}
void MeshViewer::OnKeyRelease(KeyCodes key, bool shift, bool ctl, bool alt){
	switch(key){
	case KeyCodes::W:
		cameraVelocity_.y = 0.0f;
		break;
	case KeyCodes::S:
		cameraVelocity_.y = 0.0f;
		break;
	case KeyCodes::A:
		cameraVelocity_.x = 0.0f;
		break;
	case KeyCodes::D:
		cameraVelocity_.x = 0.0f;
		break;
	case KeyCodes::Q:
		cameraVelocity_.z = 0.0f;
		break;
	case KeyCodes::E:
		cameraVelocity_.z = 0.0f;
		break;
	
	}
}


void MeshViewer::OnUpdate(double deltaTime, double totalTime){
	//static double elapsedSeconds =0.0;
	//static uint64_t frameCounter=0;
	//frameCounter++;
	//elapsedSeconds += deltaTime;
	//if(elapsedSeconds>1.0){
	//	auto fps = frameCounter/elapsedSeconds;
	//	DebugPrint("FPS: %f\n",fps);
	//	elapsedSeconds = 0;
	//	frameCounter = 0;
	//}
	gameState_ = nextState_;

	ImGui_ImplDX12_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();

	switch(gameState_){
	case GameStates::VIEWER:
	{
		CenterMesh();
		ScaleMesh();

		DirectX::XMStoreFloat4(&cameraPos_, DirectX::XMVectorAdd(DirectX::XMVectorScale(DirectX::XMLoadFloat4(&cameraVelocity_), deltaTime),DirectX::XMLoadFloat4(&cameraPos_)));
		const DirectX::XMVECTOR eyePostition = DirectX::XMLoadFloat4(&cameraPos_);
		const DirectX::XMVECTOR focusPoint = DirectX::XMVectorSet(0, 0, 0, 1);
		const DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(0, 1, 0, 0);
		viewMatrix_ = DirectX::XMMatrixLookToLH(eyePostition, DirectX::XMVectorSet(0, 0, 1, 0), upDirection);

		float aspectRatio = GetClientWidth()/static_cast<float>(GetClientHeight());
		projectionMatrix_ = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.0f), aspectRatio, 0.1f, 100.0f);


		float angle = static_cast<float>(totalTime*0.0/200.0);
		const DirectX::XMVECTOR rotationAxis = DirectX::XMVectorSet(0, 1, 0, 0);
		DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationAxis(rotationAxis, angle);
		modelMatrix_ = DirectX::XMMatrixMultiply(modelMatrix_, rotationMatrix);
	}
	break;
	case GameStates::SPLASH:
	{
		SplashUI();
	}
	break;
	case GameStates::LOADING:
	{
		if(!threadSpawned_){
			asyncThread_.Start(&MeshViewer::ProcessObjFile,this, std::cref(filePath_));
			threadSpawned_ = true;
		}
		if(asyncThread_.GetCompleted()){
			asyncThread_.Join();
			asyncThread_.Reset();
			nextState_ = GameStates::VIEWER;
		}
		LoadingUI();
	}
	break;
	}

}


void MeshViewer::OnRender(double deltaTime, double totalTime){
	ImGui::Render();
	ID3D12Resource* pBackBuffer = pWindow->GetCurrentBackBuffer();
	UINT currentBackBufferIdx = pWindow->GetCurrentBackbufferIndex();
	ID3D12GraphicsCommandList2* pCommandList = pApp_->GetCommandQueue()->GetCommandList();
	D3D12_CPU_DESCRIPTOR_HANDLE dsv = pDsvHeap_->GetCPUDescriptorHandleForHeapStart();
	D3D12_CPU_DESCRIPTOR_HANDLE rtv = pWindow->GetCurrentRenderTargetView();

	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(pBackBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	pCommandList->ResourceBarrier(1, &barrier);

	FLOAT clearColor[] = {0.0f,0.0f,0.0f,1.0f};
	pCommandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	pCommandList->ClearDepthStencilView(dsv,D3D12_CLEAR_FLAG_DEPTH,1.0f,0,0,nullptr);

	pCommandList->SetGraphicsRootSignature(pRootSignature_);

	D3D12_VIEWPORT viewPort = CD3DX12_VIEWPORT(0.0f, 0.0f, static_cast<float>(GetClientWidth()), static_cast<float>(GetClientHeight()));
	pCommandList->RSSetViewports(1,&viewPort);

	D3D12_RECT scissorRect = CD3DX12_RECT(0,0, LONG_MAX, LONG_MAX);
	pCommandList->RSSetScissorRects(1, &scissorRect);

	pCommandList->OMSetRenderTargets(1, &rtv, FALSE, &dsv);

	switch(gameState_){
	case GameStates::VIEWER:
	{
	DirectX::XMMATRIX mvpMatrix = DirectX::XMMatrixMultiply(modelMatrix_, DirectX::XMMatrixScaling(zoom_,zoom_,zoom_));
	mvpMatrix = DirectX::XMMatrixMultiply(mvpMatrix, viewMatrix_);
	mvpMatrix = DirectX::XMMatrixMultiply(mvpMatrix, projectionMatrix_);
	pCommandList->SetGraphicsRoot32BitConstants(0, sizeof(DirectX::XMMATRIX)/4, &mvpMatrix,0);
	pCommandList->SetGraphicsRoot32BitConstants(0, sizeof(DirectX::XMMATRIX)/4, &modelMatrix_,16);

	RecordMainRenderPass(pCommandList, occluderOffsetData_.at(occluderRankingData_.at(meshIdx).second));
	if(boundingBoxVisible_){
		//RecordDebugRenderPass(pCommandList);
	}
	}
	break;
	case GameStates::SPLASH:
	{
	
	}
	break;
	case GameStates::LOADING:
	{
	}
	break;
	}
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



void MeshViewer::UpdateBufferResource(
	ID3D12GraphicsCommandList2 *pCommandList,
	ID3D12Resource **ppDestinationResource,
	ID3D12Resource **ppStagingResource,
	size_t numElements,
	size_t elementSize,
	const void *buffer,
	D3D12_RESOURCE_FLAGS flags){

	ID3D12Device2 *pDevice = pApp_->GetDevice();
	size_t bufferSize = elementSize*numElements;

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&::CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_ID3D12Resource,
		reinterpret_cast<void **>(ppDestinationResource)));


	if(buffer){
		ThrowIfFailed(pDevice->CreateCommittedResource(
			&::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
			D3D12_HEAP_FLAG_NONE,
			&::CD3DX12_RESOURCE_DESC::Buffer(bufferSize),
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_ID3D12Resource,
			reinterpret_cast<void **>(ppStagingResource)
		));
		
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = buffer;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;


		::UpdateSubresources(pCommandList, *ppDestinationResource, *ppStagingResource, 0, 0, 1, &subresourceData);

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

	ThrowIfFailed(pDevice->CreateCommittedResource(
		&::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&::CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_D32_FLOAT, width, height, 1, 0, 1,0, D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL),
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

void MeshViewer::UploadMainPassResources(const std::vector<VertexPosTexNorm> &indexedVertexData, const std::vector<uint32_t> &indexData) {
	CommandQueue *pCommandQueue = pApp_->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	ID3D12GraphicsCommandList2 *pCommandList = pCommandQueue->GetCommandList(); 
	ID3D12Resource *pStagingVertexBuffer =nullptr;
	UpdateBufferResource(pCommandList, &pVertexBuffer_[0], &pStagingVertexBuffer,
		indexedVertexData.size(), sizeof(VertexPosTexNorm), indexedVertexData.data(), D3D12_RESOURCE_FLAG_NONE);

	vertexBufferView_[0].BufferLocation = pVertexBuffer_[0]->GetGPUVirtualAddress();
	vertexBufferView_[0].SizeInBytes = (UINT)indexedVertexData.size()*sizeof(VertexPosTexNorm);
	vertexBufferView_[0].StrideInBytes = sizeof(VertexPosTexNorm);

	ID3D12Resource *pStagingIndexBuffer = nullptr;
	UpdateBufferResource(pCommandList, &pIndexBuffer_[0], &pStagingIndexBuffer,
		indexData.size(), sizeof(uint32_t), indexData.data(), D3D12_RESOURCE_FLAG_NONE);
	indexBufferView_[0].BufferLocation = pIndexBuffer_[0]->GetGPUVirtualAddress();
	indexBufferView_[0].SizeInBytes = sizeof(uint32_t)*(UINT)indexData.size();
	indexBufferView_[0].Format = DXGI_FORMAT_R32_UINT;

	uint64_t fence= pCommandQueue->ExecuteCommandList(pCommandList);
	pCommandQueue->WaitForFenceValue(fence);

	SafeRelease(pStagingVertexBuffer);
	SafeRelease(pStagingIndexBuffer);

}


void MeshViewer::CreateMainPassPipelineState(){
	ID3D12Device2 *pDevice = pApp_->GetDevice();
	std::vector<char> vertShader;
	std::string filePath = SHADERS_PATH;
	filePath.append("vs.cso");
	FileTools::LoadFileToBuffer(filePath, &vertShader);

	std::vector<char> pixelShader;
	filePath = SHADERS_PATH;
	filePath.append("ps.cso");
	FileTools::LoadFileToBuffer(filePath, &pixelShader);
	

	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData = {};
	featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if(FAILED(pDevice->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData, sizeof(featureData)))) featureData.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;

	D3D12_ROOT_SIGNATURE_FLAGS rootSignatureFlags =
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_GEOMETRY_SHADER_ROOT_ACCESS|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_DOMAIN_SHADER_ROOT_ACCESS|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_HULL_SHADER_ROOT_ACCESS|
		D3D12_ROOT_SIGNATURE_FLAG_DENY_PIXEL_SHADER_ROOT_ACCESS;
		
	CD3DX12_ROOT_PARAMETER1 rootParameter[1];
	rootParameter[0].InitAsConstants(2*sizeof(DirectX::XMMATRIX)/4,0,0,D3D12_SHADER_VISIBILITY_VERTEX);

	CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC rootSignatureDesc;
	rootSignatureDesc.Init_1_1(_countof(rootParameter), rootParameter, 0, nullptr, rootSignatureFlags);

	ID3DBlob *pRootSignatureBlob;
	ID3DBlob *pErrorBlob;
	ThrowIfFailed(::D3DX12SerializeVersionedRootSignature(&rootSignatureDesc, featureData.HighestVersion, &pRootSignatureBlob, &pErrorBlob));
	ThrowIfFailed(pDevice->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_ID3D12RootSignature, reinterpret_cast<void **>(&pRootSignature_)));

	SafeRelease(pRootSignatureBlob);
	SafeRelease(pErrorBlob);

	struct PipelineStateStream{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
		CD3DX12_PIPELINE_STATE_STREAM_VS vs;
		CD3DX12_PIPELINE_STATE_STREAM_PS ps;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormat;

	} pipelineStateStream;
	

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = pRootSignature_;
	pipelineStateStream.inputLayout = {VertexPosTexNorm::inputLayout, _countof(VertexPosTexNorm::inputLayout)};
	pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE((void *)vertShader.data(), vertShader.size());
	pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE((void *)pixelShader.data(), pixelShader.size());
	pipelineStateStream.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.rtvFormat = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(pipelineStateStream), &pipelineStateStream
	};

	ThrowIfFailed(pDevice->CreatePipelineState(&pipelineStateStreamDesc, IID_ID3D12PipelineState, reinterpret_cast<void **>(&pPipelineState_[0])));
}

void MeshViewer::UploadDebugPassResources(const std::vector<VertexPos> &indexedVertexData, const std::vector<uint32_t> &indexData){
	CommandQueue *pCommandQueue = pApp_->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	ID3D12GraphicsCommandList2 *pCommandList = pCommandQueue->GetCommandList(); 
	ID3D12Resource *pStagingVertexBuffer =nullptr;
	UpdateBufferResource(pCommandList, &pVertexBuffer_[1], &pStagingVertexBuffer,
		indexedVertexData.size(), sizeof(VertexPos), indexedVertexData.data(), D3D12_RESOURCE_FLAG_NONE);

	vertexBufferView_[1].BufferLocation = pVertexBuffer_[1]->GetGPUVirtualAddress();
	vertexBufferView_[1].SizeInBytes = (UINT)indexedVertexData.size()*sizeof(VertexPos);
	vertexBufferView_[1].StrideInBytes = sizeof(VertexPos);

	ID3D12Resource *pStagingIndexBuffer = nullptr;
	UpdateBufferResource(pCommandList, &pIndexBuffer_[1], &pStagingIndexBuffer,
		indexData.size(), sizeof(uint32_t), indexData.data(), D3D12_RESOURCE_FLAG_NONE);
	indexBufferView_[1].BufferLocation = pIndexBuffer_[1]->GetGPUVirtualAddress();
	indexBufferView_[1].SizeInBytes = sizeof(uint32_t)*(UINT)indexData.size();
	indexBufferView_[1].Format = DXGI_FORMAT_R32_UINT;

	uint64_t fence= pCommandQueue->ExecuteCommandList(pCommandList);
	pCommandQueue->WaitForFenceValue(fence);

	SafeRelease(pStagingVertexBuffer);
	SafeRelease(pStagingIndexBuffer);

}

void MeshViewer::CreateDebugPassPipelineState(){
	ID3D12Device2 *pDevice = pApp_->GetDevice();
	std::vector<char> vertShader;
	std::string filePath = SHADERS_PATH;
	filePath.append("vs_debug.cso");
	FileTools::LoadFileToBuffer(filePath, &vertShader);

	std::vector<char> pixelShader;
	filePath = SHADERS_PATH;
	filePath.append("ps_debug.cso");
	FileTools::LoadFileToBuffer(filePath, &pixelShader);
	
 
	struct PipelineStateStream{
		CD3DX12_PIPELINE_STATE_STREAM_ROOT_SIGNATURE pRootSignature;
		CD3DX12_PIPELINE_STATE_STREAM_INPUT_LAYOUT inputLayout;
		CD3DX12_PIPELINE_STATE_STREAM_PRIMITIVE_TOPOLOGY primitiveTopology;
		CD3DX12_PIPELINE_STATE_STREAM_VS vs;
		CD3DX12_PIPELINE_STATE_STREAM_PS ps;
		CD3DX12_PIPELINE_STATE_STREAM_DEPTH_STENCIL_FORMAT dsvFormat;
		CD3DX12_PIPELINE_STATE_STREAM_RENDER_TARGET_FORMATS rtvFormat;

	} pipelineStateStream;
	

	D3D12_RT_FORMAT_ARRAY rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream.pRootSignature = pRootSignature_;
	pipelineStateStream.inputLayout = {VertexPos::inputLayout, _countof(VertexPos::inputLayout)};
	pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE((void *)vertShader.data(), vertShader.size());
	pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE((void *)pixelShader.data(), pixelShader.size());
	pipelineStateStream.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.rtvFormat = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(pipelineStateStream), &pipelineStateStream
	};

	ThrowIfFailed(pDevice->CreatePipelineState(&pipelineStateStreamDesc, IID_ID3D12PipelineState, reinterpret_cast<void **>(&pPipelineState_[1])));

}
void MeshViewer::RecordMainRenderPass(ID3D12GraphicsCommandList2 *pCommandList, OccluderMesh &meshInfo) const{
	pCommandList->SetPipelineState(pPipelineState_[0]);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0,1, &vertexBufferView_[0]);
	pCommandList->IASetIndexBuffer(&indexBufferView_[0]);
	pCommandList->DrawIndexedInstanced((UINT)meshInfo.numIndicesTotal, 1, meshInfo.indexOffset, 0, 0);
}
void MeshViewer::RecordDebugRenderPass(ID3D12GraphicsCommandList2 *pCommandList) const{
	pCommandList->SetPipelineState(pPipelineState_[1]);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView_[1]);
	pCommandList->IASetIndexBuffer(&indexBufferView_[1]);
	pCommandList->DrawIndexedInstanced((UINT)indexBufferView_[1].SizeInBytes/sizeof(uint32_t),1, 0, 0,0);

}

void MeshViewer::ScaleMesh(){
	using namespace DirectX;
	AABB boundingBox = occluderOffsetData_.at(occluderRankingData_.at(meshIdx).second).boundingBox;
	struct BoundingBox2D{
		XMFLOAT2 max;
		XMFLOAT2 min;
	};
	std::vector<VertexPos> bbVertices;
	boundingBox.Vertices(&bbVertices);
	XMMATRIX mvp = XMMatrixMultiply(modelMatrix_, viewMatrix_);
	mvp = XMMatrixMultiply(mvp, projectionMatrix_);
	BoundingBox2D bbScreen = {XMFLOAT2(-FLT_MAX,-FLT_MAX),XMFLOAT2(FLT_MAX,FLT_MAX)};
	
	for(auto iter = bbVertices.begin(); iter!=bbVertices.end(); ++iter){
		XMFLOAT4 vert = {iter->vert.x,iter->vert.y,iter->vert.z, 1.0f};
		XMFLOAT4 ssVert;
		XMStoreFloat4(&ssVert ,XMVector4Transform(XMLoadFloat4(&vert), mvp));
		XMFLOAT2 ssPos = {ssVert.x/ssVert.w,ssVert.y/ssVert.w};
		bbScreen.min = {std::fminf(bbScreen.min.x, ssPos.x),std::fminf(bbScreen.min.y, ssPos.y)};
		bbScreen.max = {std::fmaxf(bbScreen.max.x, ssPos.x),std::fmaxf(bbScreen.max.y, ssPos.y)};
	}
	XMFLOAT2 len;
	XMStoreFloat2(&len,XMVector2Length(XMVectorSubtract(XMLoadFloat2(&bbScreen.max), XMLoadFloat2(&bbScreen.min))));
	float scaleFactor = 1/len.x;
	XMMATRIX scaleMatrix = XMMatrixScaling(scaleFactor,scaleFactor,scaleFactor);
	modelMatrix_ = XMMatrixMultiply(modelMatrix_, scaleMatrix);
}

void MeshViewer::CenterMesh(){
	using namespace DirectX;
	enum Direction:uint8_t{
		X=0,
		Y=1,
		Z=2
	};
	AABB boundingBox = occluderOffsetData_.at(occluderRankingData_.at(meshIdx).second).boundingBox;
	XMFLOAT3 bbCenter;
	boundingBox.Center(&bbCenter);
	XMMATRIX translationMatrix= XMMatrixTranslationFromVector(XMVectorNegate(XMLoadFloat3(&bbCenter)));
	modelMatrix_ = translationMatrix;

	//XMFLOAT3 bbEdgeLength;
	//XMStoreFloat3(&bbEdgeLength, XMVectorSubtract(XMLoadFloat3(&boundingBox.max), XMLoadFloat3(&boundingBox.min)));

	//uint8_t rotationAxis = Direction::X;
	//if(bbEdgeLength.y<bbEdgeLength.z&&bbEdgeLength.y<bbEdgeLength.x) rotationAxis  =Direction::Y;
	//if(bbEdgeLength.z<bbEdgeLength.x&&bbEdgeLength.z<bbEdgeLength.y) rotationAxis = Direction::Z;
	//XMMATRIX rotationMatrix = XMMatrixIdentity();
	//if(rotationAxis==Direction::X){
	//	rotationMatrix=XMMatrixRotationX(90);
	//}
	//if(rotationAxis==Direction::Y){
	//	rotationMatrix = XMMatrixRotationY(90);
	//}

	//modelMatrix_ = XMMatrixMultiply(modelMatrix_, rotationMatrix);
}

void MeshViewer::ProcessObjFile(const std::string &filePath,JobState &state){
	std::vector<VertexPosTexNorm> indexedVertexData = {};
	std::vector<uint32_t> indexData = {};
	std::vector<MaterialInfo> materialInfoData;
	std::unordered_map<std::string, size_t> materialIdMap;
	state.stage = 0;
	FileTools::Obj obj(filePath_);
	obj.MapFile();
	obj.ParseObjFile(&indexedVertexData, &indexData, &subMeshData_, &materialInfoData, &materialIdMap);
	obj.CloseFile();
	
	DebugPrint("Triangles: %i\n", indexData.size());
	//DebugPrint("LOD Triangles: %i\n", lodData.size());
	state.stage = 1;
	std::string currObject = "";
	std::string currGroup = "";
	std::string currMaterial = "";
	SubMesh temp;
	for(uint32_t i = 0; i<subMeshData_.size(); ++i){
		state.percent = float(i)/subMeshData_.size();
		temp = subMeshData_[i];
		if(temp.objName!=currObject){
			occluderOffsetData_.push_back({temp.objName, temp.indexOffset, temp.numIndices, temp.numIndices,  temp.vertexOffset, temp.numVertices,temp.numVertices, 0.0f, AABB()});
			currObject = temp.objName;
			continue;
		}
		if(temp.groupName!=currGroup&&temp.objName==""){
			occluderOffsetData_.push_back({temp.groupName, temp.indexOffset, temp.numIndices, temp.numIndices,  temp.vertexOffset, temp.numVertices,temp.numVertices, 0.0f, AABB()});
			currGroup = temp.groupName;
			continue;
		}
		if(temp.material!=currMaterial&&temp.objName==""&&temp.groupName==""){
			occluderOffsetData_.push_back({temp.material, temp.indexOffset, temp.numIndices, temp.numIndices,  temp.vertexOffset, temp.numVertices,temp.numVertices, 0.0f, AABB()});
			currMaterial = temp.material;
			continue;
		}
		occluderOffsetData_.back().numIndicesTotal += temp.numIndices;
		occluderOffsetData_.back().numVerticesTotal += temp.numVertices;
		if(!temp.alphaTested){
			occluderOffsetData_.back().numIndices += temp.numIndices;
			occluderOffsetData_.back().numVertices += temp.numVertices;
		}
	
	}




	size_t listSize = occluderOffsetData_.size();
	DataAnalysis::DataAnalyzer analyzer(listSize);
	std::vector<std::string_view> itemList(listSize);
	std::vector<float> lengthScaleData(listSize);
	std::vector<float> occluderScoreData(listSize);
	std::vector<float> triangleNumData(listSize);
 
	std::vector<AABB> maxBoundingBoxData = {};
	std::vector<AABB> minBoundingBoxData = {};
	//std::vector<VertexPos> indexedBBVertexData = {};
	//std::vector<uint32_t> bbIndexData = {};
	state.stage = 2;
	for(size_t idx=0; idx<occluderOffsetData_.size(); ++idx){
		state.percent = (float)idx/occluderOffsetData_.size();
		maxBoundingBoxData.clear();
		minBoundingBoxData.clear();
		OccluderMesh *pMeshInfo = &occluderOffsetData_[idx];
		double exp = std::log2((double)pMeshInfo->numIndices/30.0);
		size_t maxLimit = (size_t)std::pow(2, (size_t)exp);
		size_t minLimit = (size_t)std::pow(2, (size_t)(exp/4));
		MeshTools::GenerateAABBData(indexedVertexData.data(), indexedVertexData.size(), indexData.data()+pMeshInfo->indexOffset, pMeshInfo->numIndices, maxLimit, &maxBoundingBoxData);
		MeshTools::GenerateAABBData(indexedVertexData.data(), indexedVertexData.size(), indexData.data()+pMeshInfo->indexOffset, pMeshInfo->numIndices, minLimit, &minBoundingBoxData);
		//MeshTools::GenerateAABBWireFrame(maxBoundingBoxData,indexedBBVertexData, bbIndexData);
		float occluderScore = MeshTools::GetOccluderPotential(minBoundingBoxData, maxBoundingBoxData, pMeshInfo->numIndicesTotal/3);
		AABB boundingBox = maxBoundingBoxData.back();
		pMeshInfo->occluderScore = occluderScore;
		pMeshInfo->boundingBox = boundingBox;
		//DebugPrint("Occluder Potential: %f\n", pMeshInfo->occluderScore);	
		itemList[idx] = pMeshInfo->meshId;
		lengthScaleData[idx] = boundingBox.GetDiagonal();
		occluderScoreData[idx] = (occluderScore)? occluderScore*boundingBox.GetAABBSurfaceArea() : 0.0f;
		triangleNumData[idx] = pMeshInfo->numIndicesTotal/3;
	}

	analyzer.SetItemList(std::move(itemList));
	analyzer.AddDataSeries(std::string("Length Scale"), std::move(lengthScaleData));
	analyzer.AddDataSeries(std::string("Occluder Score"), std::move(occluderScoreData));
	analyzer.AddDataSeries(std::string("Triangle Number"), std::move(triangleNumData));

	AnalyzeSceneData(analyzer);
	UploadMainPassResources(indexedVertexData,indexData);
}

void MeshViewer::AnalyzeSceneData(DataAnalysis::DataAnalyzer &analyzer){
	//analyzer.SortBySeries(std::string("Length Scale"));
	//analyzer.Truncate(1.8f, FLT_MAX);
	analyzer.SortBySeries(std::string("Occluder Score"));
	analyzer.ReturnCurrentSeries(&occluderRankingData_);
}

void MeshViewer::InitImgui(){
	ImGui_ImplWin32_EnableDpiAwareness();
	float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{0, 0}, MONITOR_DEFAULTTOPRIMARY));
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();

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
	ImGui::DestroyContext();
}

void MeshViewer::UpdateImgui(){
	ImGuiIO& io =  ImGui::GetIO();
	(void)io;

	{
		static float f = 0.0f;
		static int counter = 0;
		ImGui::Begin("Hello, world!");                          // Create a window called "Hello, world!" and append into it.

		ImGui::Text("This is some useful text.");               // Display some text (you can use a format strings too)

		ImGui::SliderFloat("float", &f, 0.0f, 1.0f);            // Edit 1 float using a slider from 0.0f to 1.0f

		if(ImGui::Button("Button"))                            // Buttons return true when clicked (most widgets return true when edited/activated)
			counter++;
		ImGui::SameLine();
		ImGui::Text("counter = %d", counter);

		ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f/io.Framerate, io.Framerate);
		ImGui::End();
	}
}

void MeshViewer::SplashUI(){
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos(ImVec2(pWindow->GetClientWidth()/2, pWindow->GetClientHeight()/2),0, ImVec2(0.5f,0.5f));
	ImGui::Begin("Splash", nullptr ,flags);
	
	if(ImGui::Button("Choose File")){
		ThrowIfFailed(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
		IFileOpenDialog *pFileOpen = NULL;
		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));
		if(SUCCEEDED(hr)){
			hr = pFileOpen->Show(pWindow->GetWindowHandle());
			if(SUCCEEDED(hr)){
				IShellItem *pItem;
				hr = pFileOpen->GetResult(&pItem);
				if(SUCCEEDED(hr)){
					PWSTR filePath = NULL;
					pItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
					std::wstring tmp(filePath);
					WStringToString(tmp, &filePath_);
					

					CoTaskMemFree(filePath);
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		::CoUninitialize();
	}
	if(filePath_!=""){
		ImGui::Text(filePath_.data());
		if(ImGui::Button("Load File")){
			nextState_ = GameStates::LOADING;
		}
	}
	ImGui::End();

}

void MeshViewer::LoadingUI(){
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos(ImVec2(pWindow->GetClientWidth()/2, pWindow->GetClientHeight()/2),0, ImVec2(0.5f,0.5f));
	ImGui::Begin("Loading", nullptr ,flags);
	switch(asyncThread_.GetStage()){
	case 0:
	ImGui::Text("Parsing File");
		break;
	case 1:
	ImGui::Text("Grouping Meshes");
		break;
	case 2:
	ImGui::Text("Calculating Occluder Data");
	ImGui::ProgressBar(asyncThread_.GetPercent(), ImVec2(-1, 0));
		break;
	}
	ImGui::End();
}
