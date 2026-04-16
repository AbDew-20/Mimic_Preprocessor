#pragma once
#include <string>
#include <cstdint>
#include <Core/AABB.h>
#include <cstddef>



struct OccluderMesh{
	std::string meshId = "";
	uint64_t indexOffset = 0ULL;
	uint64_t vertexOffset = 0ULL;
	uint32_t numIndices = 0UL;
	uint32_t numIndicesTotal = 0UL;
	uint32_t numVertices = 0UL;
	uint32_t numVerticesTotal = 0UL;
	AABB boundingBox = {};
	float occluderScore = 0.0f;

	OccluderMesh(
		std::string meshId,
		uint64_t indexOffset,
		uint64_t vertexOffset,
		uint32_t numIndices,
		uint32_t numIndicesTotal,
		uint32_t numVertices,
		uint32_t numVerticesTotal,
		AABB boundingBox,
		float occluderScore
		):
		 meshId(meshId),
		 indexOffset(indexOffset),
		 vertexOffset(vertexOffset),
		 numIndices(numIndices),
		 numIndicesTotal(numIndicesTotal),
		 numVertices(numVertices),
		 numVerticesTotal(numVerticesTotal),
		 boundingBox(boundingBox),
		 occluderScore(occluderScore)
	{
	}
};

struct SubMesh{
	std::string objName = "";
	std::string groupName = "";
	std::string material = "";
	uint64_t indexOffset = 0ULL;
	uint64_t vertexOffset = 0ULL;
	uint32_t numIndices = 0UL;
	uint32_t numVertices = 0UL;
	bool alphaTested = false;
};

struct MeshInfo{
	std::string meshId = "";
	uint64_t indexOffset = 0ULL ;
	uint64_t vertexOffset = 0ULL;
	uint32_t numIndices = 0UL;
	uint32_t numVertices = 0UL;
	AABB boundingBox = {};
	float occluderScore = 0.0f;
};
