#include <Core/Application.h>
#include <dxgidebug.h>
#include <Apps/MeshViewer.h>

void LiveObjects(){
	IDXGIDebug1 *dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL,DXGI_DEBUG_RLO_DETAIL);
	dxgiDebug->Release();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int cmdShow){
	Application app(hInstance);
	const std::wstring windowName = L"Mimic Engine";
	int ret = 0;
	app.Init();
	{
		std::string filePath = RESOURCES_PATH;
		filePath.append("Bunny/bunny.obj");
		MeshViewer screen(&app, windowName, 1200, 720, filePath, true);
		ret = app.Run(&screen);
	}
	app.ShutDown();
	::atexit(&LiveObjects);
	return ret;

}