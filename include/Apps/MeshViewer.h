#pragma once
#include <Core/Game.h>
#include <Core/VertexTypes.h>
#include <DirectXMath.h>
#include <Core/Window.h>
#include <Utils/FileTools.h>


class Application;
class CommandQueue;
struct MeshInfo;
class MeshViewer : public Game{
	
public:
	using super = Game;
	MeshViewer(Application *pApp,const std::wstring &name, int width, int height,const std::string &filePath, bool vSync = false);


	virtual bool LoadContent() override;

	virtual void UnloadContent() override;
protected:
	virtual void OnUpdate(double deltaTime, double totalTime) override;
	virtual void OnRender(double deltaTime, double totalTime) override;
	virtual void OnResize(int height, int width) override;
	virtual void OnKeyPress(KeyCodes key, bool shift, bool ctl, bool alt) override;
	virtual void OnKeyRelease(KeyCodes key, bool shift, bool ctl, bool alt) override;

	virtual void OnWindowDestroy();

private:
	void UpdateBufferResource(ID3D12GraphicsCommandList2 *pCommandList,
		ID3D12Resource **ppDestinationResource,
		ID3D12Resource **ppStagingResource,
		size_t numElements, size_t elementSize,
		const void *buffer, D3D12_RESOURCE_FLAGS flags);
	void CreateDepthBuffer(int width, int height);

	void UploadMainPassResources(const std::vector<VertexPosTexNorm> &indexedVertexData,
		const std::vector<uint32_t> &indexData);
	void CreateMainPassPipelineState();

	void RecordMainRenderPass(ID3D12GraphicsCommandList2 *pCommandList, MeshInfo &meshInfo) const;
	void RecordDebugRenderPass(ID3D12GraphicsCommandList2 *pCommandList) const;

	void UploadDebugPassResources(
		const std::vector<VertexPos> &indexedVertexData,
		const std::vector<uint32_t> &indexData);

	void CreateDebugPassPipelineState();

	void SreenSpaceSize(const AABB &boundingBox) const;

	inline constexpr float GetCameraSpeed(){ return 1.0f; }


	Application *pApp_;
	uint64_t fenceValues_[Window::kBufferCount] = {};
	const std::string filePath_;
	std::vector<MeshInfo> meshOffsetData_;

	ID3D12Resource *pDepthBuffer_;
	ID3D12DescriptorHeap *pDsvHeap_;

	ID3D12Resource *pVertexBuffer_[2];
	D3D12_VERTEX_BUFFER_VIEW vertexBufferView_[2];

	ID3D12Resource *pIndexBuffer_[2];
	D3D12_INDEX_BUFFER_VIEW indexBufferView_[2];

	ID3D12RootSignature *pRootSignature_;
	ID3D12PipelineState *pPipelineState_[2];

	DirectX::XMMATRIX viewMatrix_;
	DirectX::XMMATRIX modelMatrix_;
	DirectX::XMMATRIX projectionMatrix_;
	DirectX::XMFLOAT4 cameraVelocity_;
	DirectX::XMFLOAT4 cameraPos_;

	bool boundingBoxVisible_;
	size_t meshIdx;
	float zoom_;



};