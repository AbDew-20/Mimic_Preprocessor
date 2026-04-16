#pragma once
#include <Core/MeshTypes.h>
#include <vector>

struct Scene{
	std::vector<OccluderMesh> meshData;
	uint32_t numOccluders;
};