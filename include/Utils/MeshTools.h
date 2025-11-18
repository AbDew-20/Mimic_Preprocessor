#pragma once
#include <DirectXMath.h>
#include <vector>
#include <Core/VertexTypes.h>
#include <Core/AABB.h>

namespace MeshTools{
	void SimplifyMesh(const VertexPosTexNorm *pIndexedVertexData, size_t numVertices, const uint32_t *pIndexData, size_t numIndices, std::vector<uint32_t> *pLodData);
	void GenerateAABBData(const VertexPosTexNorm *pIndexedVertexData, uint64_t numVertices,  const uint32_t *pIndexData, uint64_t numIndices, size_t limit, std::vector<AABB> *pBoundingBoxData);
	void GenerateAABBWireFrame(const std::vector<AABB> &boundingBoxData, std::vector<VertexPos> *pVertexData, std::vector<uint32_t> *pIndexData);
	float GetOccluderPotential(const std::vector<AABB> &minBoundingBoxData, const std::vector<AABB> &maxBoundingBoxData, size_t numTriangles);
	AABB GetAABB(const VertexPosTexNorm *pIndexedVertexData, uint64_t numVertices);

}
