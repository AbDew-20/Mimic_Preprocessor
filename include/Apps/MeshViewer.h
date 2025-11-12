#pragma once
#include <Core/Game.h>
#include <Core/Window.h>
#include <Core/FileLoader.h>
#include <Core/CommandQueue.h>


class Application;
class MeshViewer : public Game{
	
public:
	using super = Game;
	MeshViewer(Application *pApp,const std::wstring &name, int width, int height,const std::string &filePath, bool vSync = false);


	virtual bool LoadContent() override;

	virtual void UnloadContent() override;
protected:
	virtual void OnUpdate(double deltaTime, double totalTime) override;
	virtual void OnRender(double deltaTime, double totalTime) override;

	virtual void OnWindowDestroy();

private:
	void UpdateBufferResource(ID3D12GraphicsCommandList2 *pCommandList, ID3D12Resource **ppDestinationResource, ID3D12Resource **ppStagingResource, size_t numElements, size_t elementSize, const void *buffer, D3D12_RESOURCE_FLAGS flags);
	void CreateDepthBuffer(int width, int height);


	Application *pApp_;
	uint64_t fenceValues_[Window::kBufferCount] = {};
	const std::string &filePath_;

	ID3D12Resource *pDepthBuffer_;
	ID3D12DescriptorHeap *pDsvHeap_;

	ID3D12Resource *pVertexBuffer_;
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_;

	ID3D12Resource *pIndexBuffer_;
	D3D12_INDEX_BUFFER_VIEW indexBufferView_;

	ID3D12RootSignature *pRootSignature_;
	ID3D12PipelineState *pPipelineState_;

	DirectX::XMMATRIX viewMatrix_;
	DirectX::XMMATRIX modelMatrix_;
	DirectX::XMMATRIX projectionMatrix_;



};