#include <BlankScreen.h>
#include <Application.h>
#include <PCH.h>




BlankScreen::BlankScreen(Application *pApp,const std::wstring &name, int width, int height, bool vSync):
	super(pApp,	name, width, height, vSync),
	pApp_(pApp)
{
}


bool BlankScreen::LoadContent(){ return true; }

void BlankScreen::UnloadContent(){}


void BlankScreen::OnUpdate(double deltaTime, double totalTime){
	static double elapsedSeconds =0.0;
	static uint64_t frameCounter=0;
	frameCounter++;
	elapsedSeconds += deltaTime;
	if(elapsedSeconds>1.0){
		char buffer[500];
		auto fps = frameCounter/elapsedSeconds;
		sprintf_s(buffer, 500, "FPS: %f\n", fps);
		::OutputDebugStringA(buffer);
		elapsedSeconds = 0;
		frameCounter = 0;
	}
}


void BlankScreen::OnRender(double deltaTime, double totalTime){
	auto backBuffer = pWindow->GetCurrentBackBuffer();
	auto commandList = pApp_->GetCommandQueue()->GetCommandList();
	CD3DX12_RESOURCE_BARRIER barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET);
	commandList->ResourceBarrier(1, &barrier);
	FLOAT clearColor[] = {0.4f,0.6f,0.9f,1.0f};
	auto rtv = pWindow->GetCurrentRenderTargetView();
	commandList->ClearRenderTargetView(rtv, clearColor, 0, nullptr);
	barrier = CD3DX12_RESOURCE_BARRIER::Transition(backBuffer, D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT);
	commandList->ResourceBarrier(1, &barrier);
	int fenceValue =pApp_->GetCommandQueue()->ExecuteCommandList(commandList);
	pWindow->Present();
	pApp_->GetCommandQueue()->WaitForFenceValue(fenceValue);
}

void BlankScreen::OnWindowDestroy(){
	pWindow = nullptr;
}