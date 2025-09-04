#pragma once
#include <Game.h>
#include <Window.h>


class Application;
class BlankScreen : public Game{
	
public:
	using super = Game;
	BlankScreen(Application *pApp,const std::wstring &name, int width, int height, bool vSync = false);


	virtual bool LoadContent() override;

	virtual void UnloadContent() override;
protected:
	virtual void OnUpdate(double deltaTime, double totalTime) override;
	virtual void OnRender(double deltaTime, double totalTime) override;

	virtual void OnWindowDestroy();

private:
	Application *pApp_;



};