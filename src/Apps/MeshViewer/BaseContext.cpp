#include <Apps/MeshViewer/BaseContext.h>

BaseContext::BaseContext(){

}


void BaseContext::Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4* pCommandList,double deltaTime){
	const float clearColour[4] = {1.0f,0.0f,0.0f,1.0f};
	CD3DX12_CLEAR_VALUE clearValue = {DXGI_FORMAT_R32G32B32_FLOAT, clearColour};

	D3D12_RENDER_PASS_BEGINNING_ACCESS rtvBeginingAccess = {D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_CLEAR, {clearValue}};
	D3D12_RENDER_PASS_ENDING_ACCESS rtvEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_PRESERVE, {}};
	D3D12_RENDER_PASS_RENDER_TARGET_DESC rtvDescriptor = {D3D12_RENDER_PASS_RENDER_TARGET_DESC{rtv, rtvBeginingAccess, rtvEndingAccess}};

	D3D12_RENDER_PASS_BEGINNING_ACCESS dsvBeginingAccess = {D3D12_RENDER_PASS_BEGINNING_ACCESS_TYPE_NO_ACCESS, {}};
	D3D12_RENDER_PASS_ENDING_ACCESS dsvEndingAccess = {D3D12_RENDER_PASS_ENDING_ACCESS_TYPE_NO_ACCESS, {}};
	D3D12_RENDER_PASS_DEPTH_STENCIL_DESC dsvDescriptor = {D3D12_RENDER_PASS_DEPTH_STENCIL_DESC{dsv, dsvBeginingAccess,dsvBeginingAccess, dsvEndingAccess, dsvEndingAccess}};
	pCommandList->BeginRenderPass(1, &rtvDescriptor, &dsvDescriptor, D3D12_RENDER_PASS_FLAG_NONE);
	pCommandList->EndRenderPass();
}

void BaseContext::Update(double deltaTime){
	
}

void BaseContext::HandleInput(MappedInput &mappedInput){

}
