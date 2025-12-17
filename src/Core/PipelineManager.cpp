#include <Core/PipelineManager.h>
#include <Core/Application.h>

PipelineManager::PipelineManager(Application *pApp) :
	pApp_(pApp)
{
}

void PipelineManager::Init(){
	featureData_.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_1;
	if(FAILED(pApp_->GetDevice()->CheckFeatureSupport(D3D12_FEATURE_ROOT_SIGNATURE, &featureData_, sizeof(featureData_)))) featureData_.HighestVersion = D3D_ROOT_SIGNATURE_VERSION_1_0;
}

ID3D12RootSignature* PipelineManager::CreateRootSignature(
	const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &rootDesc)
{
	ID3D12RootSignature *pRootSignature = nullptr;
	ID3DBlob *pRootSignatureBlob;
	ID3DBlob *pErrorBlob;
	ThrowIfFailed(::D3DX12SerializeVersionedRootSignature(&rootDesc, featureData_.HighestVersion, &pRootSignatureBlob, &pErrorBlob));
	ThrowIfFailed(pApp_->GetDevice()->CreateRootSignature(0, pRootSignatureBlob->GetBufferPointer(), pRootSignatureBlob->GetBufferSize(), IID_ID3D12RootSignature, reinterpret_cast<void **>(&pRootSignature)));

	SafeRelease(pRootSignatureBlob);
	SafeRelease(pErrorBlob);
	rootSignatures_.push_back(pRootSignature);
	return pRootSignature;
}

ID3D12PipelineState *PipelineManager::CreatePipelineState(const D3D12_PIPELINE_STATE_STREAM_DESC &pipelineStateStreamDesc){
	ID3D12PipelineState *pPipelineState;
	ThrowIfFailed(pApp_->GetDevice()->CreatePipelineState(&pipelineStateStreamDesc, IID_ID3D12PipelineState, reinterpret_cast<void **>(&pPipelineState)));
	pipelineStateObjects_.push_back(pPipelineState);
	return pPipelineState;
}

void PipelineManager::Cleanup(){
	for(ID3D12PipelineState* pPso : pipelineStateObjects_){
		SafeRelease(pPso);
	}

	for(ID3D12RootSignature *pRootSignature : rootSignatures_){
		SafeRelease(pRootSignature);
	}
}