#pragma once
#include <string>
#include <cstdint>
#include <Core/AABB.h>



struct OccluderMesh{
	std::string meshId;
	uint64_t indexOffset;
	uint32_t numIndices;
	uint32_t numIndicesTotal;
	uint64_t vertexOffset;
	uint32_t numVertices;
	uint32_t numVerticesTotal;
	float occluderScore;
	AABB boundingBox;
};

struct SubMesh{
	std::string objName;
	std::string groupName;
	std::string material;
	uint64_t indexOffset;
	uint64_t vertexOffset;
	uint32_t numIndices;
	uint32_t numVertices;
	bool alphaTested;
};

struct MeshInfo{
	std::string meshId;
	uint64_t indexOffset;
	uint32_t numIndices;
	uint64_t vertexOffset;
	uint32_t numVertices;
	float occluderScore;
	AABB boundingBox;
};
