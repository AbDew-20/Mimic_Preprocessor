#pragma once
#include <Core/PCH.h>
#include <Core/AsyncJob.h>
#include <Utils/DataAnalysis.h>
#include <Core/ResourceManager.h>
#include <Core/PipelineManager.h>
#include <DirectXMath.h>
#include <Core/VertexTypes.h>
#include <Core/AABB.h>
#include <Utils/FileTools.h>

struct ViewerStateParams{
	int clientWidth;
	int clientHeight;
	bool loading;
	bool &asyncStarted;
	std::string &workType;
};
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
struct MappedInput;
class Application;
class ViewerContext{
public:
	ViewerContext(Application* pApp, const std::string &filePath, AsyncJob &asyncThread);
	void Update(ViewerStateParams &stateParams,double deltaTime, double totalTime);
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4* pCommandList,double deltaTime);
	void HandleInput(MappedInput &mappedInput);
	void Load();
	void Unload();
protected:

private:
	enum class RenderPass{
		MAIN,
		DEBUG
	};
	void CenterMesh();
	void ScaleMesh();
	void CreatePipelines();

	void AnalyzeSceneData(DataAnalysis::DataAnalyzer &analyzer);
	void ProcessObjFile(
		const std::string &filePath,
		std::vector<VertexPosTexNorm> &indexedVertexData,
		std::vector<uint32_t> &indexData,
		std::vector<VertexPos> &bbVertexData,
		std::vector<uint32_t> &bbIndexData,
		JobState &state);

	inline constexpr float GetCameraSpeed(){ return 1.0f; }
	std::string filePath_;
	AsyncJob &asyncThread_;
	Application *pApp_;
		
	std::vector<VertexPosTexNorm> indexedVertexData_;
	std::vector<uint32_t> indexData_;
	std::vector<SubMesh> subMeshData_;

	std::vector<VertexPos> bbVertexData_;
	std::vector<uint32_t> bbIndexData_;

	std::vector<OccluderMesh> occluderOffsetData_;
	std::vector<std::pair<float, size_t>> occluderRankingData_;

	BufferResource vertexBuffers_[2];
	BufferResource indexBuffers_[2];

	Pipeline pipelines_[2];

	DirectX::XMMATRIX viewMatrix_;
	DirectX::XMMATRIX modelMatrix_;
	DirectX::XMMATRIX projectionMatrix_;
	DirectX::XMFLOAT4 cameraVelocity_;
	DirectX::XMFLOAT4 cameraPos_;

	bool boundingBoxVisible_;
	size_t meshIdx;
	float zoom_;
	bool fileLoaded_ = false;
	bool buffersUploaded_ = false;
};
