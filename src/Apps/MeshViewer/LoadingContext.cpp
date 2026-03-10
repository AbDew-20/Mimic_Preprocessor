#include <Apps/MeshViewer/LoadingContext.h>
#include <imgui.h>

LoadingContext::LoadingContext(const std::string &label):
	label_(label)
{
}


void LoadingContext::Update(const LoadingUpdateParams &updateParams, double deltatime){
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos(ImVec2(updateParams.clientWidth/2, updateParams.clientHeight/2),0, ImVec2(0.5f,0.5f));
	ImGui::Begin("Loading", nullptr ,flags);
	ImGui::Text(updateParams.label.data());
	if(updateParams.percent>0){
		ImGui::ProgressBar(updateParams.percent, ImVec2(-1, 0));
	}
	ImGui::End();
}
void LoadingContext::Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4 *pCommandList, double deltaTime){


}