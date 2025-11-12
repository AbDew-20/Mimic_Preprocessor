#pragma once
#include <Core/Game.h>
#include <Core/Window.h>


class Application;
class BlankScreen : public Game{
	
public:
	using super = Game;
	BlankScreen(Application *pApp,const std::wstring &name, int width, int height,const std::string &filePath, bool vSync = false);


	virtual bool LoadContent() override;

	virtual void UnloadContent() override;
protected:
	virtual void OnUpdate(double deltaTime, double totalTime) override;
	virtual void OnRender(double deltaTime, double totalTime) override;

	virtual void OnWindowDestroy();

private:
	Application *pApp_;
	const std::string &filePath_;



};