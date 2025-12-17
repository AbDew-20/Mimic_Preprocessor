#pragma once

#include <string>
#include <memory>
#include <Core/KeyCodes.h>
class Window;
class Application;


class Game{
public:
	Game(Application *pApp, const std::wstring &name, int width, int height, bool vSync);
	~Game();

	inline int GetClientHeight(){
		return height_;
	}
	inline int GetClientWidth(){
		return width_;
	}
	virtual bool Initialize();

	virtual bool LoadContent() = 0;
	
	virtual void UnloadContent() = 0;
	virtual void Destroy();
	virtual void TransitionState() = 0;

protected:
	friend class Window;
	virtual void OnUpdate(double deltaTime, double totalTime);
	virtual void OnRender(double deltaTime, double totalTime);
	virtual void OnResize(int height, int width);
	virtual void OnKeyPress(KeyCodes key, bool shift, bool ctl, bool alt);
	virtual void OnKeyRelease(KeyCodes key, bool shift, bool ctl, bool alt);
	virtual void OnWindowDestroy();
	Window *pWindow;
private:
	std::wstring name_;
	int width_;
	int height_;
	bool vSync_;
	Application *pApp_;
};