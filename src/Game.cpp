#include <PCH.h>
#include <Application.h>
#include <Window.h>
#include <Game.h>


Game::Game(Application *pApp, const std::wstring &name, int width, int height, bool vSync) :
	pApp_(pApp),
	name_(name),
	width_(width),
	height_(height),
	vSync_(vSync){}

Game::~Game(){
	assert(!pWindow&&"Associated window hasn't been destroyed");
}
bool Game::Initialize(){
	if(!DirectX::XMVerifyCPUSupport()){
		MessageBoxA(NULL, "Failed to verify DirectX Math", "Error", MB_OK|MB_ICONERROR);
		return false;
	}
	pWindow = pApp_->CreateRenderWindow(name_, width_, height_, vSync_);
	pWindow->RegisterCallbacks(this);
	pWindow->Show();
	return true;
}

void Game::Destroy(){
	pApp_->DestroyWindow(pWindow);
	pWindow = nullptr;
}

void Game::OnResize(int height, int width){
	height_ = height;
	width_ = width;
}

void Game::OnRender(double deltaTime, double totalTime){}
void Game::OnUpdate(double deltaTime, double totalTime){}
void Game::OnWindowDestroy(){
	UnloadContent();
}