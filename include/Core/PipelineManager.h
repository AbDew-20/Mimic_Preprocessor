#pragma once
#include <Core/PCH.h>
#include <vector>

struct Pipeline{
	ID3D12RootSignature *pRootSignature;
	ID3D12PipelineState *pPipelineState;
	D3D12_PRIMITIVE_TOPOLOGY primitiveTopology;
};
class Application;
class PipelineManager{
public:
	PipelineManager(Application *pApp);

	ID3D12RootSignature* CreateRootSignature(const CD3DX12_VERSIONED_ROOT_SIGNATURE_DESC &rootDesc);
	ID3D12PipelineState *CreatePipelineState(const D3D12_PIPELINE_STATE_STREAM_DESC &pipelineStateStreamDesc);
	void Init();
	void Cleanup();
protected:

private:
	Application *pApp_;
	D3D12_FEATURE_DATA_ROOT_SIGNATURE featureData_;
	std::vector<ID3D12RootSignature *> rootSignatures_;
	std::vector<ID3D12PipelineState *> pipelineStateObjects_;

};