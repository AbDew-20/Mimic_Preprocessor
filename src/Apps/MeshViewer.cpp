#include <Apps/MeshViewer.h>
#include <Core/Application.h>
#include <Core/PCH.h>
#include <Core/CommandQueue.h>
#include <Utils/ScopedTimer.h>
#include <Utils/FileLoader.h>
#include <Utils/MeshTools.h>




MeshViewer::MeshViewer(Application *pApp,const std::wstring &name, int width, int height, const std::string& filePath, bool vSync):
	super(pApp,	name, width, height, vSync),
	pApp_(pApp),
	filePath_(filePath),
	pDepthBuffer_(nullptr),
	pDsvHeap_(nullptr)
{
}


bool MeshViewer::LoadContent(){
	std::vector<VertexPosTexNorm> indexedVertexData = {};
	std::vector<uint32_t> indexData = {};
	std::vector<uint32_t> lodData = {};
	std::string materialFile;
	{
		ScopedTimer timer("File Parse");
		FileLoader::ParseObjFile(filePath_, indexedVertexData, indexData,materialFile);
	}
	MeshTools::SimplifyMesh(indexedVertexData,indexData,lodData);
	DebugPrint("Triangles: %i\n", indexData.size());
	DebugPrint("LOD Triangles: %i\n", lodData.size());

	std::vector<AABB> maxBoundingBoxData = {};
	std::vector<AABB> minBoundingBoxData = {};
	std::vector<VertexPos> indexedBBVertexData = {};
	std::vector<uint32_t> bbIndexData = {};
	{
		ScopedTimer timer("AABB");
		double exp =std::log2((double)lodData.size()/30.0);
		size_t maxLimit = (size_t)std::pow(2, (size_t)exp);
		size_t minLimit = (size_t)std::pow(2, (size_t)(exp/2));
		MeshTools::GenerateAABBData(indexedVertexData, lodData, maxBoundingBoxData, maxLimit);
		MeshTools::GenerateAABBData(indexedVertexData, lodData, minBoundingBoxData, minLimit);
		MeshTools::GenerateAABBWireFrame(maxBoundingBoxData,indexedBBVertexData, bbIndexData);
	}
	float occluderPotential = MeshTools::GetOccluderPotential(minBoundingBoxData, maxBoundingBoxData, lodData.size()/3);
	DebugPrint("Occluder Potential: %f\n", occluderPotential);

	ID3D12Device2 *pDevice = pApp_->GetDevice(); 

	UploadMainPassResources(indexedVertexData,lodData);
	UploadDebugPassResources(indexedBBVertexData, bbIndexData);

	D3D12_DESCRIPTOR_HEAP_DESC dsvHeapDesc = {};
	dsvHeapDesc.NumDescriptors = 1;
	dsvHeapDesc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsvHeapDesc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	ThrowIfFailed(pDevice->CreateDescriptorHeap(&dsvHeapDesc, IID_ID3D12DescriptorHeap, reinterpret_cast<void **>(&pDsvHeap_))); 

	CreateMainPassPipelineState();
	CreateDebugPassPipelineState();
	CreateDepthBuffer(GetClientWidth(), GetClientHeight()); 


	return true;
}

void MeshViewer::UnloadContent(){
	SafeRelease(pDepthBuffer_);
	SafeRelease(pPipelineState_[0]);
	SafeRelease(pPipelineState_[1]);
	SafeRelease(pRootSignature_);
	SafeRelease(pDsvHeap_);
	SafeRelease(pIndexBuffer_[0]);
	SafeRelease(pVertexBuffer_[0]);
	SafeRelease(pIndexBuffer_[1]);
	SafeRelease(pVertexBuffer_[1]);
}


void MeshViewer::OnUpdate(double deltaTime, double totalTime){
	static double elapsedSeconds =0.0;
	static uint64_t frameCounter=0;
	frameCounter++;
	elapsedSeconds += deltaTime;
	if(elapsedSeconds>1.0){
		auto fps = frameCounter/elapsedSeconds;
		DebugPrint("FPS: %f\n",fps);
		elapsedSeconds = 0;
		frameCounter = 0;
	}

	float angle = static_cast<float>(totalTime*90.0/200.0);
	const DirectX::XMVECTOR rotationAxis = DirectX::XMVectorSet(0, 1, 0, 0);
	modelMatrix_ = DirectX::XMMatrixRotationAxis(rotationAxis, angle);

	const DirectX::XMVECTOR eyePostition = DirectX::XMVectorSet(0, 0, -5, 0);
	const DirectX::XMVECTOR focusPoint = DirectX::XMVectorSet(0, 0, 0, 1);
	const DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(0, 1, 0, 0);
	viewMatrix_ = DirectX::XMMatrixLookAtLH(eyePostition, focusPoint, upDirection);

	float aspectRatio = GetClientWidth()/static_cast<float>(GetClientHeight());
	projectionMatrix_ = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.0f), aspectRatio, 0.1f, 100.0f);


}


void MeshViewer::OnRender(double deltaTime, double totalTime){
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

	DirectX::XMMATRIX mvpMatrix = DirectX::XMMatrixMultiply(modelMatrix_, viewMatrix_);
	mvpMatrix = DirectX::XMMatrixMultiply(mvpMatrix, projectionMatrix_);
	pCommandList->SetGraphicsRoot32BitConstants(0, sizeof(DirectX::XMMATRIX)/4, &mvpMatrix,0);
	pCommandList->SetGraphicsRoot32BitConstants(0, sizeof(DirectX::XMMATRIX)/4, &modelMatrix_,16);

	RecordMainRenderPass(pCommandList);
	RecordDebugRenderPass(pCommandList);

	barrier = CD3DX12_RESOURCE_BARRIER::Transition(pBackBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	pCommandList->ResourceBarrier(1, &barrier);
	fenceValues_[currentBackBufferIdx] = pApp_->GetCommandQueue()->ExecuteCommandList(pCommandList);
	currentBackBufferIdx= pWindow->Present();
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

void MeshViewer::UploadMainPassResources(std::vector<VertexPosTexNorm> &indexedVertexData, std::vector<uint32_t> &indexData){
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
	FileLoader::LoadFileToBuffer(filePath, vertShader);

	std::vector<char> pixelShader;
	filePath = SHADERS_PATH;
	filePath.append("ps.cso");
	FileLoader::LoadFileToBuffer(filePath, pixelShader);
	

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

void MeshViewer::UploadDebugPassResources(std::vector<VertexPos> &indexedVertexData, std::vector<uint32_t> &indexData){
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
	FileLoader::LoadFileToBuffer(filePath, vertShader);

	std::vector<char> pixelShader;
	filePath = SHADERS_PATH;
	filePath.append("ps_debug.cso");
	FileLoader::LoadFileToBuffer(filePath, pixelShader);
	
 
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
void MeshViewer::RecordMainRenderPass(ID3D12GraphicsCommandList2 *pCommandList){
	pCommandList->SetPipelineState(pPipelineState_[0]);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	pCommandList->IASetVertexBuffers(0,1, &vertexBufferView_[0]);
	pCommandList->IASetIndexBuffer(&indexBufferView_[0]);
	pCommandList->DrawIndexedInstanced((UINT)indexBufferView_[0].SizeInBytes/sizeof(uint32_t), 1, 0, 0, 0);
}
void MeshViewer::RecordDebugRenderPass(ID3D12GraphicsCommandList2 *pCommandList){
	pCommandList->SetPipelineState(pPipelineState_[1]);
	pCommandList->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_LINELIST);
	pCommandList->IASetVertexBuffers(0, 1, &vertexBufferView_[1]);
	pCommandList->IASetIndexBuffer(&indexBufferView_[1]);
	pCommandList->DrawIndexedInstanced((UINT)indexBufferView_[1].SizeInBytes/sizeof(uint32_t),1, 0, 0,0);

}
