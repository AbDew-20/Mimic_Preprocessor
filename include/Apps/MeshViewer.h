#pragma once
#include <Core/Game.h>
#include <Core/VertexTypes.h>
#include <DirectXMath.h>
#include <Core/Window.h>
#include <Utils/FileTools.h>
#include <Utils/DataAnalysis.h>
#include <Core/DescriptorHeapAllocator.h>
#include <Core/AsyncJob.h>


class Application;
class CommandQueue;
struct SubMesh;

struct OccluderMesh{
	std::string meshId;
	uint64_t indexOffset;
	uint64_t numIndices;
	uint64_t numIndicesTotal;
	uint64_t vertexOffset;
	uint64_t numVertices;
	uint64_t numVerticesTotal;
	float occluderScore;
	AABB boundingBox;
};
enum class GameStates{
	SPLASH,
	LOADING,
	VIEWER
};
class MeshViewer : public Game{
	
public:
	using super = Game;
	MeshViewer(Application *pApp,const std::wstring &name, int width, int height, bool vSync = false);


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

	void RecordMainRenderPass(ID3D12GraphicsCommandList2 *pCommandList, OccluderMesh &meshInfo) const;
	void RecordDebugRenderPass(ID3D12GraphicsCommandList2 *pCommandList) const;

	void UploadDebugPassResources(
		const std::vector<VertexPos> &indexedVertexData,
		const std::vector<uint32_t> &indexData);

	void CreateDebugPassPipelineState();

	void InitImgui();
	void DestroyImgui();
	void UpdateImgui();
	void SplashUI();
	void ViewerUI();
	void LoadingUI();

	void CenterMesh();
	void ScaleMesh();

	void AnalyzeSceneData(DataAnalysis::DataAnalyzer &analyzer);
	void ProcessObjFile( const std::string &filePath, JobState &state);

	inline constexpr float GetCameraSpeed(){ return 1.0f; }

	GameStates gameState_;
	GameStates nextState_;
	Application *pApp_;
	uint64_t fenceValues_[Window::kBufferCount] = {};
	std::string filePath_;
	std::vector<SubMesh> subMeshData_;

	std::vector<OccluderMesh> occluderOffsetData_;
	std::vector<std::pair<float, size_t>> occluderRankingData_;

	DescriptorHeapAllocator imguiSRVAlloc_;

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

	AsyncJob asyncThread_;
	bool threadSpawned_;
	bool boundingBoxVisible_;
	size_t meshIdx;
	float zoom_;



};