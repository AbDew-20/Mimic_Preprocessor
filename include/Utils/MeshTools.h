#pragma once
#include <DirectXMath.h>
#include <vector>
#include <Core/VertexTypes.h>
struct AABB{
	DirectX::XMFLOAT3 max;
	DirectX::XMFLOAT3 min;

	AABB() = default;
};

namespace MeshTools{
	void SimplifyMesh(std::vector<VertexPosTexNorm> &indexedVertexData, std::vector<uint32_t> &indexData, std::vector<uint32_t> &lodData);
	void GenerateAABBData(std::vector<VertexPosTexNorm> &indexedVertexData, std::vector<uint32_t> &indexData, std::vector<AABB> &boundingBoxData, size_t limit=0);
	void GenerateAABBWireFrame(std::vector<AABB> &boundingBoxData, std::vector<VertexPos> &vertexData, std::vector<uint32_t> &indexData);
	float GetOccluderPotential(std::vector<AABB> &minBoundingBoxData, std::vector<AABB> &maxBoundingBoxData, size_t numTriangles);

}
