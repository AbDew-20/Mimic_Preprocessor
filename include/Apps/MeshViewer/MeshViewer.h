#pragma once
#include <Apps/MeshViewer/LoadingContext.h>
#include <Apps/MeshViewer/SplashContext.h>
#include <Apps/MeshViewer/ViewerContext.h>
#include <Apps/MeshViewer/GraphContext.h>
#include <Core/AsyncJob.h>
#include <Core/DescriptorHeapAllocator.h>
#include <Core/Game.h>
#include <Core/InputManager.h>
#include <Core/Window.h>
#include <variant>


class Application;
class CommandQueue;
struct SubMesh;

using Context = std::variant<SplashContext, ViewerContext, LoadingContext, GraphContext>;

struct SplashState{
	bool fileSelected = false;
	std::string filePath;
};
struct ViewerState{
	enum class Mode {NORMAL, LOADING} mode;
	bool asyncStarted = false;
	bool asyncFinished = false;
	std::string workType;
	bool loadGraph = false;
};
struct GraphState{
	enum class Mode {NORMAL, LOADING} mode;
	bool asyncStarted = false;
	bool asyncFinished = false;
	std::string workType;
	bool focusViewer = false;
};

using State = std::variant<SplashState, ViewerState, GraphState>;
class MeshViewer : public Game{
	
public:
	using super = Game;
	MeshViewer(Application *pApp,const std::wstring &name, int width, int height, bool vSync = false);


	virtual bool LoadContent() override;

	virtual void UnloadContent() override;
	virtual void TransitionState()override;
protected:
	friend class ViewerContext;
	virtual void OnUpdate(double deltaTime, double totalTime) override;
	virtual void OnRender(double deltaTime, double totalTime) override;
	virtual void OnResize(int height, int width) override;
	virtual void OnKeyPress(KeyCodes key, bool shift, bool ctl, bool alt) override;
	virtual void OnKeyRelease(KeyCodes key, bool shift, bool ctl, bool alt) override;
		
	virtual void OnWindowDestroy();

private:
	void CreateDepthBuffer(int width, int height);


	void HandleInput(MappedInput &input);

	void InitImgui();
	void DestroyImgui();
	void ContextCleanup();
	void LoadingUI();


	template<typename T>
	T *GetContext(){
		for(auto &context:contextStack_){
			if(auto *p = std::get_if<T>(&context)){
				return p;
			}
		}
		return nullptr;
	}

	State currentState_;
	State previousState_;
	std::vector<Context> contextStack_;

	Application *pApp_;
	uint64_t fenceValues_[Window::kBufferCount] = {};

	DescriptorHeapAllocator imguiSRVAlloc_;

	ID3D12Resource *pDepthBuffer_;
	ID3D12DescriptorHeap *pDsvHeap_;

	AsyncJob asyncThread_;
};
