#include <Apps/MeshViewer/ViewerContext.h>
#include <Apps/MeshViewer/InputContext.h>
#include <Core/InputManager.h>
#include <Core/AABB.h>
#include <Core/Application.h>
#include <Utils/MeshTools.h>
#include <Utils/StringTools.h>
#include <Utils/FileTools.h>
#include <Utils/FileTools/Pak.h>
#include <Utils/FileTools/Mvtx.h>
#include <imgui.h>


ViewerContext::ViewerContext(Application* pApp, const std::string &filePath, AsyncJob &asyncThread):
	filePath_(filePath),
	asyncThread_(asyncThread),
	pApp_(pApp),
	cameraPos_(DirectX::XMFLOAT4(0.0f, 0.0f, -5.0f, 0.0f)),
	cameraVelocity_(DirectX::XMFLOAT4(0.0f, 0.0f, 0.0f, 0.0f)),
	boundingBoxVisible_(false),
	meshIdx(0),
	zoom_(1.0f),
	pOccluderRankingData_(nullptr)
{
	vertexBuffers_[0].pBuffer = nullptr;
	vertexBuffers_[1].pBuffer = nullptr;
	indexBuffers_[0].pBuffer = nullptr;
	indexBuffers_[1].pBuffer = nullptr;
}

void ViewerContext::Load(){
	CreatePipelines();
}
void ViewerContext::Unload(){
	SafeRelease(vertexBuffers_[0].pBuffer);
	SafeRelease(indexBuffers_[0].pBuffer);
	SafeRelease(vertexBuffers_[1].pBuffer);
	SafeRelease(indexBuffers_[1].pBuffer);
}
void ViewerContext::HandleInput(MappedInput &mappedInput){
	using namespace InputContext;
	for(auto iter = mappedInput.Actions.begin(); iter!=mappedInput.Actions.end(); ++iter){
		switch(static_cast<Actions>(*iter)){
		case Actions::MoveCameraUp:
			cameraPos_.y += 0.1;
			mappedInput.ConsumeAction((size_t)Actions::MoveCameraUp);
			break;
		case Actions::MoveCameraDown:
			cameraPos_.y -= 0.1;
			mappedInput.ConsumeAction((size_t)Actions::MoveCameraDown);
			break;
		case Actions::MoveCameraLeft:
			cameraPos_.x -= 0.1;
			mappedInput.ConsumeAction((size_t)Actions::MoveCameraLeft);
			break;
		case Actions::MoveCameraRight:
			cameraPos_.x += 0.1;
			mappedInput.ConsumeAction((size_t)Actions::MoveCameraRight);
			break;
		case Actions::ZoomIn:
			zoom_ *= 2;
			mappedInput.ConsumeAction((size_t)Actions::ZoomIn);
			break;
		case Actions::ZoomOut:
			zoom_ *= 0.5;
			mappedInput.ConsumeAction((size_t)Actions::ZoomOut);
			break;
		case Actions::CycleMeshUp:
			if(meshIdx==pOccluderRankingData_->size()-1){
				meshIdx = 0;
			}
			else{
				meshIdx++;
			}
			mappedInput.ConsumeAction((size_t)Actions::CycleMeshUp);
		break;
		case Actions::CycleMeshDown:
			if(meshIdx==0){
				meshIdx = pOccluderRankingData_->size()-1;
			}
			else{
				meshIdx--;
			}
			mappedInput.ConsumeAction((size_t)Actions::CycleMeshDown);
		break;

		}
		if(mappedInput.Actions.begin()==mappedInput.Actions.end()){
			break;
		}
	
	}
	const auto statesEnd = mappedInput.States.end();
	cameraVelocity_.y = 0.0f;
	cameraVelocity_.x = 0.0f;
	cameraVelocity_.y+= (mappedInput.States.find((size_t)States::CameraMovingUp)!=statesEnd) ? +GetCameraSpeed() : 0.0f;
	cameraVelocity_.y+= (mappedInput.States.find((size_t)States::CameraMovingDown)!=statesEnd) ? -GetCameraSpeed() : 0.0f;
	cameraVelocity_.x+= (mappedInput.States.find((size_t)States::CameraMovingRight)!=statesEnd) ? +GetCameraSpeed() : 0.0f;
	cameraVelocity_.x+= (mappedInput.States.find((size_t)States::CameraMovingLeft)!=statesEnd) ? -GetCameraSpeed() : 0.0f;
}

void ViewerContext::Update(const ViewerUpdateParams &updateParams,double deltaTime, double totalTime, ViewerStateParams &stateParams){
	if(!fileLoaded_){
		if(!updateParams.loading){
			stateParams.asyncStarted = true;
			stateParams.workType = "Loading File";
			asyncThread_.Start(&ViewerContext::ProcessObjFile, this, std::cref(filePath_), std::ref(indexedVertexData_), std::ref(indexData_), std::ref(bbVertexData_), std::ref(bbIndexData_));
		}
		else{
			fileLoaded_ = asyncThread_.GetCompleted();
			uint32_t stage =asyncThread_.GetStage();
			switch(stage){
			case 0:
			stateParams.workType = "Parsing File";
				break;
			case 1:
			stateParams.workType = "Grouping Meshes";
			break;
			case 2:
			stateParams.workType = "Calculating Rating";
			break;
			}
		}
		return;
	}
	if(invalidateRanking_){
		invalidateRanking_ = false;
		meshIdx = 0;
	}
	if(!buffersUploaded_){
		ResourceManager *pResourceManager = pApp_->GetResourceManager();
		pResourceManager->UploadVertexBuffer(indexedVertexData_.data(), indexedVertexData_.size(), sizeof(indexedVertexData_[0]), &vertexBuffers_[0]);
		pResourceManager->UploadIndexBuffer(indexData_.data(), indexData_.size(), &indexBuffers_[0]);
		pResourceManager->UploadVertexBuffer(bbVertexData_.data(), bbVertexData_.size(), sizeof(bbVertexData_[0]), &vertexBuffers_[1]);
		pResourceManager->UploadIndexBuffer(bbIndexData_.data(), bbIndexData_.size(), &indexBuffers_[1]);
		buffersUploaded_ = true;
		indexedVertexData_.resize(0);
		indexData_.resize(0);
		bbVertexData_.resize(0);
		bbIndexData_.resize(0);
	}
	CenterMesh();
	ScaleMesh();

	DirectX::XMStoreFloat4(&cameraPos_, DirectX::XMVectorAdd(DirectX::XMVectorScale(DirectX::XMLoadFloat4(&cameraVelocity_), deltaTime), DirectX::XMLoadFloat4(&cameraPos_)));
	const DirectX::XMVECTOR eyePostition = DirectX::XMLoadFloat4(&cameraPos_);
	const DirectX::XMVECTOR focusPoint = DirectX::XMVectorSet(0, 0, 0, 1);
	const DirectX::XMVECTOR upDirection = DirectX::XMVectorSet(0, 1, 0, 0);
	viewMatrix_ = DirectX::XMMatrixLookToLH(eyePostition, DirectX::XMVectorSet(0, 0, 1, 0), upDirection);

	float aspectRatio = updateParams.clientWidth/static_cast<float>(updateParams.clientHeight);
	projectionMatrix_ = DirectX::XMMatrixPerspectiveFovLH(DirectX::XMConvertToRadians(45.0f), aspectRatio, 0.1f, 100.0f);


	float angle = static_cast<float>(totalTime*0.0/200.0);
	const DirectX::XMVECTOR rotationAxis = DirectX::XMVectorSet(0, 1, 0, 0);
	DirectX::XMMATRIX rotationMatrix = DirectX::XMMatrixRotationAxis(rotationAxis, angle);
	modelMatrix_ = DirectX::XMMatrixMultiply(modelMatrix_, rotationMatrix);

	ImGui::SetNextWindowSize(ImVec2(updateParams.clientWidth*0.2,updateParams.clientHeight*0.25), 0);
	ImGui::SetNextWindowPos(ImVec2{0,0});
	ImGui::Begin("Mesh Info");
	ImGui::BulletText(occluderOffsetData_[pOccluderRankingData_->at(meshIdx).second].meshId.c_str());
	ImGui::BulletText("Triangles: %i", occluderOffsetData_[pOccluderRankingData_->at(meshIdx).second].numIndices/3);
	ImGui::BulletText("Alpha tested triangles: %i", (occluderOffsetData_[pOccluderRankingData_->at(meshIdx).second].numIndicesTotal - occluderOffsetData_[pOccluderRankingData_->at(meshIdx).second].numIndices)/3);
	ImGui::BulletText("Occluder Score: %f", pOccluderRankingData_->at(meshIdx).first);
	if(ImGui::Button("Write order to file")){
		WriteOccluderRankingToFile("occluder.txt");
	}
	if(ImGui::Button("Open Graph")){
		stateParams.loadGraph = true ;
		invalidateRanking_ = true;
	}
	else{
		stateParams.loadGraph = false;
	}
	ImGui::End();
		
	
}


void ViewerContext::Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv,
	D3D12_CPU_DESCRIPTOR_HANDLE dsv,
	ID3D12GraphicsCommandList4* pCommandList,
	double deltaTime){

		const float clearColour[4] = {0.0f,0.0f,0.0f,1.0f};
		CD3DX12_CLEAR_VALUE clearValue = {DXGI_FORMAT_R32G32B32_FLOAT, clearColour};

		D3D12_RENDER_PASS_BEGINNING_ACCESS rtvBeginingAccess = {D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR, {clearValue}};
		D3D12_RENDER_PASS_ENDING_ACCESS rtvEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE, {}};
		D3D12_RENDER_PASS_RENDER_TARGET_DESC rtvDescriptor = {D3D12_RENDER_PASS_RENDER_TARGET_DESC{rtv, rtvBeginingAccess, rtvEndingAccess}};

		CD3DX12_CLEAR_VALUE depthValue = {DXGI_FORMAT_R32_FLOAT,1.0f, 0};
		D3D12_RENDER_PASS_BEGINNING_ACCESS dsvBeginingAccess = {D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR, {depthValue}};
		D3D12_RENDER_PASS_ENDING_ACCESS dsvEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE, {}};
		D3D12_RENDER_PASS_BEGINNING_ACCESS stencilBeginingAccess = {D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS, {}};
		D3D12_RENDER_PASS_ENDING_ACCESS stencilEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS, {}};
		D3D12_RENDER_PASS_DEPTH_STENCIL_DESC dsvDescriptor = {D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{dsv, dsvBeginingAccess,stencilBeginingAccess, dsvEndingAccess, stencilEndingAccess}};

		pCommandList->BeginRenderPass(1, &rtvDescriptor, &dsvDescriptor, D3D12_RENDER_PASS_FLAG_NONE);
		if(buffersUploaded_&&!invalidateRanking_){
			size_t meshIndex = pOccluderRankingData_->at(meshIdx).second;
			OccluderMesh meshInfo = occluderOffsetData_.at(meshIndex);
			DirectX::XMMATRIX mvpMatrix = DirectX::XMMatrixMultiply(modelMatrix_, DirectX::XMMatrixScaling(zoom_, zoom_, zoom_));
			mvpMatrix = DirectX::XMMatrixMultiply(mvpMatrix, viewMatrix_);
			mvpMatrix = DirectX::XMMatrixMultiply(mvpMatrix, projectionMatrix_);
			size_t passIndex = static_cast<size_t>(RenderPass::MAIN);
			pCommandList->SetGraphicsRootSignature(pipelines_[passIndex].pRootSignature);
			pCommandList->SetPipelineState(pipelines_[passIndex].pPipelineState);
			pCommandList->IASetPrimitiveTopology(pipelines_[passIndex].primitiveTopology);
			pCommandList->SetGraphicsRoot32BitConstants(0, sizeof(DirectX::XMMATRIX)/4, &mvpMatrix, 0);
			pCommandList->SetGraphicsRoot32BitConstants(0, sizeof(DirectX::XMMATRIX)/4, &modelMatrix_, 16);
			pCommandList->IASetVertexBuffers(0, 1, &vertexBuffers_[passIndex].vertexView);
			pCommandList->IASetIndexBuffer(&indexBuffers_[passIndex].indexView);
			pCommandList->DrawIndexedInstanced((UINT)meshInfo.numIndicesTotal, 1, meshInfo.indexOffset, 0, 0);


			if(boundingBoxVisible_){
				passIndex = static_cast<size_t>(RenderPass::DEBUG);
				pCommandList->SetGraphicsRootSignature(pipelines_[passIndex].pRootSignature);
				pCommandList->SetPipelineState(pipelines_[passIndex].pPipelineState);
				pCommandList->IASetPrimitiveTopology(pipelines_[passIndex].primitiveTopology);
				pCommandList->IASetVertexBuffers(0, 1, &vertexBuffers_[passIndex].vertexView);
				pCommandList->IASetIndexBuffer(&indexBuffers_[passIndex].indexView);
				pCommandList->DrawIndexedInstanced(24, 1, meshIndex*24, 0, 0);
			}
		}
		pCommandList->EndRenderPass();
	

}
void ViewerContext::ScaleMesh(){
	using namespace DirectX;
	AABB boundingBox = occluderOffsetData_.at(pOccluderRankingData_->at(meshIdx).second).boundingBox;
	struct BoundingBox2D{
		XMFLOAT2 max;
		XMFLOAT2 min;
	};
	std::vector<VertexPos> bbVertices;
	boundingBox.GetVertices(bbVertices);
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

void ViewerContext::CenterMesh(){
	using namespace DirectX;
	AABB boundingBox = occluderOffsetData_.at(pOccluderRankingData_->at(meshIdx).second).boundingBox;
	XMFLOAT3 bbCenter;
	boundingBox.GetCenter(bbCenter);
	XMMATRIX translationMatrix= XMMatrixTranslationFromVector(XMVectorNegate(XMLoadFloat3(&bbCenter)));
	translationMatrix = XMMatrixMultiply(translationMatrix, XMMatrixTranslation(0.0f, 0.0f, bbCenter.z-boundingBox.min.z));
	modelMatrix_ = translationMatrix;

	//enum Direction:uint8_t{
	//	X=0,
	//	Y=1,
	//	Z=2
	//};


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
void ViewerContext::ProcessObjFile(
	const std::string &filePath,
	std::vector<VertexPosTexNorm> &indexedVertexData,
	std::vector<uint32_t> &indexData,
	std::vector<VertexPos> &bbVertexData,
	std::vector<uint32_t> &bbIndexData,
	JobState &state){

	std::vector<MaterialInfo> materialInfoData;
	std::unordered_map<std::string, size_t> materialIdMap;
	state.stage = 0;
	state.percent = -1.0f;
	FileTools::Obj obj(filePath_);
	obj.MapFile();
	obj.ParseObjFile(indexedVertexData, indexData, subMeshData_, materialInfoData, materialIdMap);
	obj.CloseFile();
	
	DebugPrint("Triangles: %i\n", indexData.size());
	state.stage = 1;
	std::string currObject = "";
	std::string currGroup = "";
	std::string currMaterial = "";
	SubMesh temp;
	for(uint32_t i = 0; i<subMeshData_.size(); ++i){
		state.percent = float(i)/subMeshData_.size();
		temp = subMeshData_[i];
		uint32_t opaqueIndices = temp.alphaTested? 0:temp.numIndices;
		uint32_t opaqueVertices = temp.alphaTested? 0:temp.numVertices;
		if(temp.objName!=currObject){
			occluderOffsetData_.push_back({temp.objName, temp.indexOffset, opaqueIndices, (uint32_t)temp.numIndices,  temp.vertexOffset, opaqueVertices,(uint32_t)temp.numVertices, 0.0f, AABB()});
			currObject = temp.objName;
			continue;
		}
		if(temp.groupName!=currGroup&&temp.objName==""){
			occluderOffsetData_.push_back({temp.groupName, temp.indexOffset, opaqueIndices, temp.numIndices,  temp.vertexOffset, opaqueVertices,temp.numVertices, 0.0f, AABB()});
			currGroup = temp.groupName;
			continue;
		}
		if(temp.material!=currMaterial&&temp.objName==""&&temp.groupName==""){
			occluderOffsetData_.push_back({temp.material, temp.indexOffset, opaqueIndices, temp.numIndices,  temp.vertexOffset, opaqueVertices,temp.numVertices, 0.0f, AABB()});
			currMaterial = temp.material;
			continue;
		}
		occluderOffsetData_.back().numIndicesTotal += temp.numIndices;
		occluderOffsetData_.back().numVerticesTotal += temp.numVertices;
		occluderOffsetData_.back().numIndices += opaqueIndices;
		occluderOffsetData_.back().numVertices += opaqueVertices;
	
	}

	size_t listSize = occluderOffsetData_.size();
	analyzer_.Init(listSize);
	std::vector<std::string_view> itemList(listSize);
	std::vector<float> lengthScaleData(listSize);
	std::vector<float> occluderScoreData(listSize);
	std::vector<float> triangleNumData(listSize);
 
	std::vector<AABB> maxBoundingBoxData = {};
	std::vector<AABB> minBoundingBoxData = {};
	state.stage = 2;
	for(size_t idx=0; idx<occluderOffsetData_.size(); ++idx){
		state.percent = (float)idx/occluderOffsetData_.size();
		maxBoundingBoxData.clear();
		minBoundingBoxData.clear();
		OccluderMesh *pMeshInfo = &occluderOffsetData_[idx];
		double exp = std::log2((double)pMeshInfo->numIndices/30.0);
		size_t maxLimit = (size_t)std::pow(2, (size_t)exp);
		size_t minLimit = (size_t)std::pow(2, (size_t)(exp/4));
		if(pMeshInfo->numIndices>0){
			MeshTools::GenerateAABBData(indexedVertexData.data(), indexedVertexData.size(), indexData.data()+pMeshInfo->indexOffset, pMeshInfo->numIndices, maxLimit, &maxBoundingBoxData);
			MeshTools::GenerateAABBData(indexedVertexData.data(), indexedVertexData.size(), indexData.data()+pMeshInfo->indexOffset, pMeshInfo->numIndices, minLimit, &minBoundingBoxData);
			MeshTools::PushBackMeshAABBWireFrame(maxBoundingBoxData.back(), bbVertexData, bbIndexData);
			float occluderScore = MeshTools::GetOccluderPotential(minBoundingBoxData, maxBoundingBoxData, pMeshInfo->numIndicesTotal/3);
			AABB boundingBox = maxBoundingBoxData.back();
			pMeshInfo->occluderScore = occluderScore;
			pMeshInfo->boundingBox = boundingBox;
		}
		else{
			pMeshInfo->boundingBox = MeshTools::GetAABB(indexedVertexData.data()+ pMeshInfo->vertexOffset,pMeshInfo->numVerticesTotal);
			pMeshInfo->occluderScore = 0.0f;
		}
		itemList[idx] = pMeshInfo->meshId;
		lengthScaleData[idx] = pMeshInfo->boundingBox.GetDiagonalLength();
		occluderScoreData[idx] = pMeshInfo->occluderScore*pMeshInfo->boundingBox.GetAABBSurfaceArea();
		triangleNumData[idx] = pMeshInfo->numIndicesTotal/3.0f;
	}

	analyzer_.SetItemList(std::move(itemList));
	analyzer_.AddDataSeries(std::string("Length Scale"), std::move(lengthScaleData));
	analyzer_.AddDataSeries(std::string("Occluder Score"), std::move(occluderScoreData));
	analyzer_.AddDataSeries(std::string("Triangle Number"), std::move(triangleNumData));

	AnalyzeSceneData(analyzer_);
	fileLoaded_ = true;
}

void ViewerContext::AnalyzeSceneData(DataAnalysis::DataAnalyzer &analyzer){
	analyzer.SortBySeries("Occluder Score");
	pOccluderRankingData_= analyzer.ReturnCurrentSeriesP();
}

void ViewerContext::CreatePipelines(){
	PipelineManager* pPipelineManager =	pApp_->GetPipelineManager(); 	


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

	pipelines_[0].pRootSignature = pPipelineManager->CreateRootSignature(rootSignatureDesc);
	pipelines_[1].pRootSignature = pipelines_[0].pRootSignature;
	
	pipelines_[0].primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	pipelines_[1].primitiveTopology = D3D_PRIMITIVE_TOPOLOGY_LINELIST;

	std::vector<char> vertShader;
	std::string filePath = SHADERS_PATH;
	filePath.append("vs.cso");
	FileTools::LoadFileToBuffer(filePath, &vertShader);

	std::vector<char> pixelShader;
	filePath = SHADERS_PATH;
	filePath.append("ps.cso");
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

	pipelineStateStream.pRootSignature = pipelines_[0].pRootSignature;
	pipelineStateStream.inputLayout = {VertexPosTexNorm::inputLayout, _countof(VertexPosTexNorm::inputLayout)};
	pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE((void *)vertShader.data(), vertShader.size());
	pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE((void *)pixelShader.data(), pixelShader.size());
	pipelineStateStream.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.rtvFormat = rtvFormats;

	D3D12_PIPELINE_STATE_STREAM_DESC pipelineStateStreamDesc = {
		sizeof(pipelineStateStream), &pipelineStateStream
	};
	pipelines_[0].pPipelineState=pPipelineManager->CreatePipelineState(pipelineStateStreamDesc);

	vertShader.clear();
	filePath = SHADERS_PATH;
	filePath.append("vs_debug.cso");
	FileTools::LoadFileToBuffer(filePath, &vertShader);

	pixelShader.clear();
	filePath = SHADERS_PATH;
	filePath.append("ps_debug.cso");
	FileTools::LoadFileToBuffer(filePath, &pixelShader);

	rtvFormats = {};
	rtvFormats.NumRenderTargets = 1;
	rtvFormats.RTFormats[0] = DXGI_FORMAT_R8G8B8A8_UNORM;

	pipelineStateStream = {};
	pipelineStateStream.pRootSignature = pipelines_[1].pRootSignature;
	pipelineStateStream.inputLayout = {VertexPos::inputLayout, _countof(VertexPos::inputLayout)};
	pipelineStateStream.primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_LINE;
	pipelineStateStream.vs = CD3DX12_SHADER_BYTECODE((void *)vertShader.data(), vertShader.size());
	pipelineStateStream.ps = CD3DX12_SHADER_BYTECODE((void *)pixelShader.data(), pixelShader.size());
	pipelineStateStream.dsvFormat = DXGI_FORMAT_D32_FLOAT;
	pipelineStateStream.rtvFormat = rtvFormats;

	pipelineStateStreamDesc = {
		sizeof(pipelineStateStream), &pipelineStateStream
	};
	pipelines_[1].pPipelineState = pPipelineManager->CreatePipelineState(pipelineStateStreamDesc);
}
void ViewerContext::WriteOccluderRankingToFile(const std::string &fileName){
	std::vector<std::string_view> tokens;
	StringTools::ParseString(filePath_, '\\', &tokens);
	std::string filePath ="";
	for(int i = 0; i<tokens.size()-1; ++i){
		filePath.append(tokens[i]);
		filePath.append("\\");
	}
	filePath.append(fileName);
	std::vector<char> meshIds;
	for(int i = 0; i<pOccluderRankingData_->size(); ++i){
		std::string_view meshId(occluderOffsetData_[pOccluderRankingData_->at(i).second].meshId);
		meshIds.reserve(meshIds.size()+meshId.size()+1);
		for(int j = 0; j<meshId.size(); ++j){
			meshIds.push_back(meshId[j]);
		}
		meshIds.push_back('\n');
	}
	FileTools::WriteBufferToFile(filePath, meshIds);
}

void ViewerContext::SerializeBuffers(){
	std::string file("Test.pak");
	std::string dir(RESOURCES_PATH);
	FileTools::Pak pak(file,dir);
	pak.OpenPak();
	auto writeStream = pak.OpenItemWriteStream("mesh.mvtx");
	FileTools::MVertex mvtx;
	std::byte *byteArray = (std::byte *)indexedVertexData_.data();
	size_t arraySizeInBytes = indexedVertexData_.size()*sizeof(VertexPosTexNorm);
	std::vector<std::byte>vertexData(byteArray,byteArray+arraySizeInBytes);
	VertexLayout layout = {sizeof(VertexPosTexNorm), std::vector<VertexAttributeDesc>(std::begin(VertexPosTexNorm::attributeData), std::end(VertexPosTexNorm::attributeData))};
	mvtx.Serialize(vertexData, indexData_, layout, writeStream);
	pak.CloseItemWriteStream();
	pak.ClosePak();
}

void ViewerContext::DeserializeBuffers(){
	std::string file("Test.pak");
	std::string dir(RESOURCES_PATH);
	FileTools::Pak pak(file,dir);
	pak.OpenPak();
	std::vector<FileTools::PakItemInfo> pakInfo = pak.ReturnPakInfo();
	auto readStream = pak.OpenItemReadStream(0);
	FileTools::MVertex mvtx;
	std::vector<std::byte> vertexData;
	std::vector<uint32_t> indexData;
	VertexLayout layout;
	mvtx.Deserialize(vertexData, indexData, layout, readStream);
	pak.CloseItemReadStream();
	pak.ClosePak();
	if(VertexTypes::matches<VertexPosTexNorm>(layout)){
		VertexPosTexNorm *data = (VertexPosTexNorm*)vertexData.data();
		std::vector<VertexPosTexNorm> vertexTypedData(data, data+(vertexData.size()/layout.stride));
	}
}