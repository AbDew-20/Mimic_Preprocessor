#pragma once
#include <Core/PCH.h>
#include <Core/AsyncJob.h>
#include <Utils/DataAnalysis.h>
#include <Core/ResourceManager.h>
#include <Core/PipelineManager.h>
#include <DirectXMath.h>
#include <Core/VertexTypes.h>
#include <Core/MeshTypes.h>

struct ViewerStateParams{
	bool asyncStarted;
	std::string workType;
	bool loadGraph;
};
struct ViewerUpdateParams{
	int clientWidth;
	int clientHeight;
	bool loading;
};
struct MappedInput;
class Application;
class ViewerContext{
public:
	ViewerContext(Application* pApp, const std::string &filePath, AsyncJob &asyncThread);
	void Update(const ViewerUpdateParams &updateParams,double deltaTime, double totalTime, ViewerStateParams &stateParams);
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4* pCommandList,double deltaTime);
	void HandleInput(MappedInput &mappedInput);
	void Load();
	void Unload();
	DataAnalysis::DataAnalyzer& GetAnalyzer(){
		return analyzer_;
	}
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

	void WriteOccluderRankingToFile(const std::string &fileName);
	void SerializeBuffers();
	void DeserializeBuffers();

	inline constexpr float GetCameraSpeed(){ return 1.0f; }
	std::string filePath_;
	AsyncJob &asyncThread_;
	Application *pApp_;
		
	std::vector<VertexPosTexNorm> indexedVertexData_;
	std::vector<uint32_t> indexData_;
	std::vector<SubMesh> subMeshData_;

	std::vector<VertexPos> bbVertexData_;
	std::vector<uint32_t> bbIndexData_;

	DataAnalysis::DataAnalyzer analyzer_;

	std::vector<OccluderMesh> occluderOffsetData_;
	std::vector<std::pair<float, size_t>> *pOccluderRankingData_;

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
	bool invalidateRanking_ = false;
};
