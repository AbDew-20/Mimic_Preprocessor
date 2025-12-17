#include <Apps/MeshViewer/MeshViewer.h>
#include <Apps/MeshViewer/SplashContext.h>
#include <imgui.h>
#include <ShObjIdl.h>


SplashContext::SplashContext(){
}


void SplashContext::Update(SplashStateParams* pStateParams,double deltaTime){
	ImGuiWindowFlags flags = ImGuiWindowFlags_NoResize|ImGuiWindowFlags_NoMove|ImGuiWindowFlags_AlwaysAutoResize|ImGuiWindowFlags_NoTitleBar;
	ImGui::SetNextWindowPos(ImVec2(pStateParams->clientWidth/2, pStateParams->clientHeight/2),0, ImVec2(0.5f,0.5f));
	ImGui::Begin("Splash", nullptr ,flags);
	
	if(ImGui::Button("Choose File")){
		ThrowIfFailed(::CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED));
		IFileOpenDialog *pFileOpen = NULL;
		HRESULT hr = CoCreateInstance(CLSID_FileOpenDialog, NULL, CLSCTX_INPROC_SERVER, IID_PPV_ARGS(&pFileOpen));
		if(SUCCEEDED(hr)){
			hr = pFileOpen->Show(nullptr);
			if(SUCCEEDED(hr)){
				IShellItem *pItem;
				hr = pFileOpen->GetResult(&pItem);
				if(SUCCEEDED(hr)){
					PWSTR filePath = NULL;
					pItem->GetDisplayName(SIGDN_FILESYSPATH, &filePath);
					std::wstring tmp(filePath);
					WStringToString(tmp, &pStateParams->fileName);
					

					CoTaskMemFree(filePath);
					pItem->Release();
				}
			}
			pFileOpen->Release();
		}
		::CoUninitialize();
	}
	if(pStateParams->fileName!=""){
		ImGui::Text(pStateParams->fileName.data());
		pStateParams->fileSelected= ImGui::Button("Load File");
	}
	ImGui::End();

}



void SplashContext::Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4 *pCommandList, double deltaTime){
	const float clearColour[4] = {0.0f,0.0f,0.0f,1.0f};
	CD3DX12_CLEAR_VALUE clearValue = {DXGI_FORMAT_R32G32B32_FLOAT, clearColour};

	D3D12_RENDER_PASS_BEGINNING_ACCESS rtvBeginingAccess = {D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR, {clearValue}};
	D3D12_RENDER_PASS_ENDING_ACCESS rtvEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE, {}};
	D3D12_RENDER_PASS_RENDER_TARGET_DESC rtvDescriptor = {D3D12_RENDER_PASS_RENDER_TARGET_DESC{rtv, rtvBeginingAccess, rtvEndingAccess}};

	CD3DX12_CLEAR_VALUE depthValue = {DXGI_FORMAT_R32_FLOAT,1.0f, 0};
	D3D12_RENDER_PASS_BEGINNING_ACCESS dsvBeginingAccess = {D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR, {depthValue}};
	D3D12_RENDER_PASS_ENDING_ACCESS dsvEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_DISCARD, {}};
	D3D12_RENDER_PASS_BEGINNING_ACCESS stencilBeginingAccess = {D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS, {}};
	D3D12_RENDER_PASS_ENDING_ACCESS stencilEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS, {}};
	D3D12_RENDER_PASS_DEPTH_STENCIL_DESC dsvDescriptor = {D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{dsv, dsvBeginingAccess,stencilBeginingAccess, dsvEndingAccess, stencilEndingAccess}};
	pCommandList->BeginRenderPass(1, &rtvDescriptor, &dsvDescriptor, D3D12_RENDER_PASS_FLAG_NONE);
	pCommandList->EndRenderPass();
}
