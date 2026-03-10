#pragma once
#include <Core/PCH.h>
#include <Utils/DataAnalysis.h>

struct MappedInput;
struct GraphStateParams{
	bool focusVeiwer;
};
struct GraphUpdateParams{
	int clientWidth;
	int clientHeight;
};
class GraphContext{
public:
	GraphContext(DataAnalysis::DataAnalyzer &analyzer);
	void Update(const GraphUpdateParams &updateParams,double deltaTime, GraphStateParams &stateParams);
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4* pCommandList,double deltaTime);
	void HandleInput(MappedInput &mappedInput);
	void Load(){ return; }
	void Unload(){ return; }
protected:

private:
	struct GraphStateVars{
		int lastYAxisItemSelectedIdx;
		int lastXAxisItemSelectedIdx;
		int yAxisItemSelectedIdx;
		int xAxisItemSelectedIdx;
		int selectionType;
		float valueBegin; 
		float valueEnd;
		float percentileBegin;
		float percentileEnd;
		bool updateGraph;
	};
	DataAnalysis::DataAnalyzer &analyzer_;
	GraphStateVars graphStateVariables_;
		
};
