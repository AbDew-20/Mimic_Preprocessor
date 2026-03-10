#include <Apps/MeshViewer/GraphContext.h>
#include <imgui.h>
#include <implot.h>


GraphContext::GraphContext(DataAnalysis::DataAnalyzer &analyzer):
	analyzer_(analyzer),
	graphStateVariables_({0, 1, 0, 1, 0, 0.0f, 0.0f, 10.0f, 90.0f, true}){

}
 void GraphContext::Update(const GraphUpdateParams &updateParams, double deltaTime, GraphStateParams &stateParams){
	ImGui::SetNextWindowSize(ImVec2(updateParams.clientWidth,updateParams.clientHeight), 0);
	ImGui::SetNextWindowPos(ImVec2(0, 0));
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_NoTitleBar;

	std::vector<std::string_view> seriesNames;
	seriesNames.push_back("Item Count");
	analyzer_.GetSeriesNames(seriesNames);
	ImGui::Begin("GraphViewer", nullptr, flags);
	float graphHeight = 0.45*ImGui::GetWindowSize().y;
	ImGui::BeginChild("Graph", ImVec2(0, graphHeight));
	//{
	//	ImVec2 vMin = ImGui::GetWindowContentRegionMin();
	//	ImVec2 vMax = ImGui::GetWindowContentRegionMax();

	//	vMin.x += ImGui::GetWindowPos().x;
	//	vMin.y += ImGui::GetWindowPos().y;
	//	vMax.x += ImGui::GetWindowPos().x;
	//	vMax.y += ImGui::GetWindowPos().y;

	//	ImGui::GetForegroundDrawList()->AddRect(vMin, vMax, IM_COL32(255, 255, 0, 255));
	//}
	std::vector<float> histogramx;
	std::vector<float> histogramy;
	std::vector<std::pair<float, float>> binData;
	static ImVec2 xAxisExtents(0,0);
	if(graphStateVariables_.updateGraph){
		analyzer_.SortBySeries(std::string(seriesNames[graphStateVariables_.lastXAxisItemSelectedIdx]));
		std::pair<float,float> seriesExtents =  analyzer_.GetCurrentSeriesExtents();
		xAxisExtents = {seriesExtents.first, seriesExtents.second};
		graphStateVariables_.valueBegin= xAxisExtents.x;
		graphStateVariables_.valueEnd = xAxisExtents.y;
		graphStateVariables_.updateGraph = false;
	}
	std::string yAxisSeries = (graphStateVariables_.lastYAxisItemSelectedIdx==0) ? "" : std::string(seriesNames[graphStateVariables_.lastYAxisItemSelectedIdx]);
	analyzer_.GenerateHistogram(yAxisSeries, 100, 0, FLT_MAX, histogramx, histogramy, binData);
	if(ImPlot::BeginPlot("Scene Data")){
		ImPlot::PlotBars("Test", histogramx.data(), histogramy.data(), histogramx.size(), 1.0);
		ImPlot::EndPlot();
	}
	ImGui::EndChild();

	ImGui::BeginChild("Controls", ImVec2(0, 0));
	
	float footerHeight = 0.0f;
	footerHeight += ImGui::GetFrameHeightWithSpacing();
	footerHeight += ImGui::GetStyle().ItemSpacing.y;
	footerHeight += ImGui::GetFrameHeightWithSpacing();
	footerHeight += ImGui::GetFrameHeightWithSpacing();
	footerHeight += ImGui::GetFrameHeightWithSpacing();


	ImGui::BeginChild("Left", ImVec2(ImGui::GetContentRegionAvail().x*0.3, ImGui::GetContentRegionAvail().y-footerHeight));
	ImGui::PushItemWidth(100);
	{
		ImGui::BeginGroup();
		ImGui::Text("Y-Axis");
		if(ImGui::BeginListBox("##YAxis")){
			for(int n = 0; n<seriesNames.size(); ++n){
				const bool isSelected = (graphStateVariables_.yAxisItemSelectedIdx==n);
				if(n!=graphStateVariables_.xAxisItemSelectedIdx&&ImGui::Selectable(seriesNames[n].data(), isSelected)){
					graphStateVariables_.yAxisItemSelectedIdx = n;
				}
				if(isSelected){
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
		ImGui::EndGroup();
	}

	ImGui::SameLine();
	{
		ImGui::BeginGroup();
		ImGui::Text("X-Axis");
		if(ImGui::BeginListBox("##XAxis")){
			for(int n = 1; n<seriesNames.size(); ++n){
				const bool isSelected = (graphStateVariables_.xAxisItemSelectedIdx==n);
				if(n!=graphStateVariables_.yAxisItemSelectedIdx&&ImGui::Selectable(seriesNames[n].data(), isSelected)){
					graphStateVariables_.xAxisItemSelectedIdx = n;
				}
				if(isSelected){
					ImGui::SetItemDefaultFocus();
				}
			}
			ImGui::EndListBox();
		}
		ImGui::EndGroup();
	}
	if(ImGui::Button("Update Axes")){
		graphStateVariables_.lastYAxisItemSelectedIdx = graphStateVariables_.yAxisItemSelectedIdx;
		graphStateVariables_.lastXAxisItemSelectedIdx = graphStateVariables_.xAxisItemSelectedIdx;
		graphStateVariables_.updateGraph = true;
	}

	ImGui::PopItemWidth();
	ImGui::EndChild();
	static int e=0;
	ImGui::RadioButton("Percentile",&e,0);
	ImGui::SameLine();
	ImGui::RadioButton("Value",&e,1);
	static float percentileBegin = 10, percentileEnd = 90;
	float speed = (xAxisExtents.y-xAxisExtents.x)/40.0f;
	if(e==0){
		ImGui::DragFloatRange2("Percentile Range", &percentileBegin, &percentileEnd, 0.25f, 0.0f, 100.0f, "Min: %.1f %%", "Max: %.1f %%");
	}
	else{
		ImGui::DragFloatRange2("Value Range", &graphStateVariables_.valueBegin, &graphStateVariables_.valueEnd,speed , xAxisExtents.x, xAxisExtents.y, "Min: %.1f", "Max: %.1f");
	}

	if(ImGui::Button("Truncate")){
		if(e==0){
			analyzer_.TruncateByPercentile(yAxisSeries,percentileBegin, percentileEnd);
		}
		else{
			analyzer_.TruncateByValue(graphStateVariables_.valueBegin, graphStateVariables_.valueEnd);
		}
		graphStateVariables_.updateGraph = true;
	}
	ImGui::SetItemTooltip("Truncates the data set based on raw or percentile values");

	ImGui::Separator();
	if(ImGui::Button("Auto Filter")){
		analyzer_.SortBySeries("Length Scale");
		analyzer_.TruncateByPercentile("Triangle Number", 25.0f, 100.0f);
		analyzer_.SortBySeries("Occluder Score");
		analyzer_.TruncateByPercentile("Triangle Number", 65.0f, 100.0f);
	}
	ImGui::SetItemTooltip("Automatically selects best occluders");
	ImGui::SameLine();
	if(ImGui::Button("Reset Graph")){
		analyzer_.ResetFrame();
		graphStateVariables_.updateGraph = true;
	}
	ImGui::SetItemTooltip("Resets any truncations");
	ImGui::SameLine();
	stateParams.focusVeiwer = ImGui::Button("Close Graph");
	ImGui::EndChild();
	ImGui::End();
}
void GraphContext::Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4 *pCommandList, double deltaTime){
}
void GraphContext::HandleInput(MappedInput &mappedInput){
}
