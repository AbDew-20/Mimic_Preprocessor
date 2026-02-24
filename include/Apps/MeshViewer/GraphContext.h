#pragma once
#include <Core/PCH.h>
#include <Utils/DataAnalysis.h>

struct MappedInput;
struct GraphStateParams{
	int clientWidth;
	int clientHeight;
	bool &focusVeiwer;
};
class GraphContext{
public:
	GraphContext(DataAnalysis::DataAnalyzer &analyzer);
	void Update(GraphStateParams &stateParams,double deltaTime);
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4* pCommandList,double deltaTime);
	void HandleInput(MappedInput &mappedInput);
	void Load(){ return; }
	void Unload(){ return; }
protected:

private:
	DataAnalysis::DataAnalyzer &analyzer_;
		
};
