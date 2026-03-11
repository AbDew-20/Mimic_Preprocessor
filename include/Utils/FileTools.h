#pragma once
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <string_view>
#include <Core/VertexTypes.h>
#include <Core/AABB.h>
#include <unordered_map>


struct MeshInfo{
	std::string meshId;
	uint64_t indexOffset;
	uint64_t numIndices;
	uint64_t vertexOffset;
	uint64_t numVertices;
	float occluderScore;
	AABB boundingBox;
};
struct SubMesh{
	std::string objName;
	std::string groupName;
	std::string material;
	uint64_t indexOffset;
	uint64_t numIndices;
	uint64_t vertexOffset;
	uint64_t numVertices;
	bool alphaTested;
};

struct MaterialInfo{
	bool alphaTested;
};
namespace FileTools{
	void LoadFileToBuffer(const std::string &filePath, std::vector<char> *pBuffer);
	void WriteBufferToFile(const std::string &filePath, const std::vector<char> &buffer);

	class Obj{
	public:
		Obj(const std::string &filePath);

		void MapFile();
		void ParseObjFile(
			std::vector<VertexPosTexNorm> &indexedVertexBuffer,
			std::vector<uint32_t> &indexBuffer,
			std::vector<SubMesh> &meshOffsetData,
			std::vector<MaterialInfo> &matierialInfoData,
			std::unordered_map<std::string, size_t> &materialIdMap );
		void CloseFile();
		

	protected:

	private:
		void SetView(SIZE_T offset);
		void ParseLines(size_t startOffset, size_t *pOutStartOffset,  std::vector<std::string_view> *pLines) const;
		void GenerateIndexBuffer(
			const std::vector<VertexPosTexNorm> &interleavedBuffer,
			std::vector<VertexPosTexNorm> &indexedInterleavedBuffer,
			std::vector<uint32_t> &indexBuffer) const;

		void LoadVertexData(
			const std::vector<std::string_view> &vertTokenList,
			const std::vector<DirectX::XMFLOAT3> &vertPosBuffer,
			const std::vector<DirectX::XMFLOAT2> &texCoordBuffer,
			const std::vector<DirectX::XMFLOAT3> &vertNormalBuffer,
			VertexPosTexNorm *pVertData) const;
		void PushBackData(
			const std::vector<SubMesh> &alphaTestedMeshData,
			const std::vector<VertexPosTexNorm> &alphaTestedVertexData,
			const std::vector<uint32_t> &alphaTestedIndexData,
			std::vector<VertexPosTexNorm> &indexedVertexData,
			std::vector<uint32_t> &indexData,
			std::vector<SubMesh> &meshOffsetData) const;


		DWORD allocGranularity_;
		SIZE_T pageSize_ ;
		uint64_t fileSize_;
	 	
		std::string filePath_;
		std::string directory_;
		HANDLE hFile_;
		HANDLE hMap_;
		LPVOID pView_;
		SIZE_T viewSize_;



	};
	class Mtl{
	public:
		Mtl(const std::string mtlFile, const std::string directory);
		void ParseMtlFile(std::vector<MaterialInfo> &matierialInfoData, std::unordered_map<std::string, size_t> &materialIdMap);
	protected:

	private:
		struct MaterialTextures;
		void GenerateMaterialData(const MaterialTextures &texturePaths, MaterialInfo *pMaterialInfo);
		std::string fileName_;
		std::string currentDir_;
	};

}
