#include <Application.h>
#include <Clock.h>
#include <BlankScreen.h>
#include <dxgidebug.h>

void LiveObjects(){
	IDXGIDebug1 *dxgiDebug;
	DXGIGetDebugInterface1(0, IID_PPV_ARGS(&dxgiDebug));
	dxgiDebug->ReportLiveObjects(DXGI_DEBUG_ALL,DXGI_DEBUG_RLO_DETAIL);
	dxgiDebug->Release();
}

int WINAPI wWinMain(HINSTANCE hInstance, HINSTANCE, PWSTR pCmdLine, int cmdShow){
	Application app(hInstance);
	const std::wstring windowName = L"Blank Screen";
	int ret = 0;
	app.Init();
	{
		BlankScreen screen(&app, windowName, 1280, 720, true);
		ret = app.Run(&screen);
	}
	app.ShutDown();
	::atexit(&LiveObjects);
	return ret;

}