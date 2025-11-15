#include <Utils/MeshTools.h>
#include <thirdParty/meshoptimizer/meshoptimizer.h>


namespace{
	constexpr uint8_t lineIndexData[24] = {0, 1, 0, 2, 0, 4, 5, 7, 5, 1, 5, 4, 3, 1, 3, 2, 3, 7, 6, 4, 6, 2, 6, 7 };

	struct RefinementParams {
		size_t maxTriangles;
		size_t minTriangles;
		size_t maxVertices;
	};


	void GenerateAABB(meshopt_Meshlet &meshlet, std::vector<uint32_t> &meshletVertices, std::vector<VertexPosTexNorm> &indexedVertexData, AABB &boundingBox){
		DirectX::XMFLOAT3 min(FLT_MAX,FLT_MAX,FLT_MAX);
		DirectX::XMFLOAT3 max(FLT_MIN,FLT_MIN,FLT_MIN);
		for(int i = 0; i<meshlet.vertex_count; ++i){
			DirectX::XMFLOAT3 temp = indexedVertexData.at(meshletVertices.at(meshlet.vertex_offset+i)).vert;
			
			min = DirectX::XMFLOAT3(std::fminf(temp.x,min.x),std::fminf(temp.y,min.y),std::fminf(temp.z,min.z));
			max = DirectX::XMFLOAT3(std::fmaxf(temp.x,max.x),std::fmaxf(temp.y,max.y),std::fmaxf(temp.z,max.z));
		}
		boundingBox.max = max;
		boundingBox.min = min;
	}

	float GetAABBVolume(AABB &boundingBox){
		using namespace DirectX;
		XMFLOAT3 diff;
		XMStoreFloat3(&diff, XMVectorSubtract(XMLoadFloat3(&boundingBox.max), XMLoadFloat3(&boundingBox.min)));
		float vol= diff.x*diff.y*diff.z;
		return vol;
	}

	float GetAABBSurfaceArea(AABB &boundingBox){
		using namespace DirectX;
		float area = 0.0f;
		XMFLOAT3 diff;
		XMStoreFloat3(&diff, XMVectorSubtract(XMLoadFloat3(&boundingBox.max), XMLoadFloat3(&boundingBox.min)));
		area += (2*diff.x*diff.y);
		area += (2*diff.z*diff.y);
		area += (2*diff.x*diff.z);
		
		return area;
	}

}

void MeshTools::GenerateAABBData(std::vector<VertexPosTexNorm> &indexedVertexData, std::vector<uint32_t> &indexData, std::vector<AABB> &boundingBoxData, size_t limit){
	RefinementParams refinement = {8, 8, 8};
	if(limit>32){
		refinement = {limit, limit/4, limit};
	}
	const float fillWeight = 1.0f;
	const size_t maxMeshlets = meshopt_buildMeshletsBound(indexData.size(), refinement.maxVertices, refinement.minTriangles);
	std::vector<meshopt_Meshlet> meshletData(maxMeshlets);
	std::vector<uint32_t> meshletVertices(indexData.size());
	std::vector<uint8_t> meshletTriangles(indexData.size());

	size_t meshletCount = meshopt_buildMeshletsSpatial(meshletData.data(),
		meshletVertices.data(),
		meshletTriangles.data(),
		indexData.data(),
		indexData.size(),
		&indexedVertexData[0].vert.x,
		indexedVertexData.size(),
		sizeof(VertexPosTexNorm),
		refinement.maxVertices, refinement.minTriangles,
		refinement.maxTriangles,
		fillWeight);
	AABB meshBoundingBox = {DirectX::XMFLOAT3(FLT_MIN,FLT_MIN,FLT_MIN), DirectX::XMFLOAT3(FLT_MAX,FLT_MAX,FLT_MAX)};
	for(int i = 0; i<meshletCount; ++i){
		AABB boundingBox;
		GenerateAABB(meshletData.at(i), meshletVertices, indexedVertexData, boundingBox);
		meshBoundingBox.min = DirectX::XMFLOAT3(std::fminf(meshBoundingBox.min.x, boundingBox.min.x), std::fminf(meshBoundingBox.min.y, boundingBox.min.y), std::fminf(meshBoundingBox.min.z, boundingBox.min.z));
		meshBoundingBox.max = DirectX::XMFLOAT3(std::fmaxf(meshBoundingBox.max.x, boundingBox.max.x), std::fmaxf(meshBoundingBox.max.y, boundingBox.max.y), std::fmaxf(meshBoundingBox.max.z, boundingBox.max.z));
		boundingBoxData.push_back(boundingBox);
	}
	boundingBoxData.push_back(meshBoundingBox);
	
}

void MeshTools::GenerateAABBWireFrame(std::vector<AABB> &boundingBoxData, std::vector<VertexPos> &vertexData, std::vector<uint32_t> &indexData){
	const uint8_t xMask = 0b00000001;
	const uint8_t yMask = 0b00000010;
	const uint8_t zMask = 0b00000100;
	const uint32_t stride = 8;
	for(int i = 0; i<boundingBoxData.size(); ++i){
		const AABB boundingBox = boundingBoxData.at(i);
		const uint32_t offset = (uint32_t)i*stride;
		for(uint8_t j = 0; j<8; ++j){
			VertexPos vert;
			vert.vert.x = (j&xMask) ? boundingBox.max.x : boundingBox.min.x;
			vert.vert.y = (j&yMask) ? boundingBox.max.y : boundingBox.min.y;
			vert.vert.z = (j&zMask) ? boundingBox.max.z : boundingBox.min.z;
			vertexData.push_back(vert);
		}

		for(int j = 0; j<24; ++j){
			indexData.push_back(offset+(uint32_t)lineIndexData[j]);
		}
	
	}
	
}

float MeshTools::GetOccluderPotential(std::vector<AABB> &minBoundingBoxData, std::vector<AABB> &maxBoundingBoxData, size_t numTriangles){
	float minInteriorVolume = 0.0f;
	float maxInteriorVolume = 0.0f;
	float meshSurfaceArea = 0.0f;
	AABB meshBoundingBox = maxBoundingBoxData.back();
	float meshVolume = GetAABBVolume(meshBoundingBox);
	for(size_t i = 0; i<minBoundingBoxData.size()-1; ++i){
		minInteriorVolume += GetAABBVolume(minBoundingBoxData.at(i));
	}
	meshSurfaceArea = GetAABBSurfaceArea(meshBoundingBox);
	float minVolumeRatio = minInteriorVolume/meshVolume;
	for(size_t i = 0; i<maxBoundingBoxData.size()-1; ++i){
		maxInteriorVolume += GetAABBVolume(maxBoundingBoxData.at(i));
	}
	float maxVolumeRatio = maxInteriorVolume/meshVolume;

	float linearMinVolumeRatio = (minVolumeRatio<1.0f) ? 1.0f/minVolumeRatio : minVolumeRatio;
	float linearMaxVolumeRatio = (maxVolumeRatio<1.0f) ? 1.0f/maxVolumeRatio : maxVolumeRatio;

	float normalizedLinearVolumeRatio = (linearMaxVolumeRatio+linearMinVolumeRatio)/2.0f;

	float normalizedSurfaceArea = meshSurfaceArea/std::pow(normalizedLinearVolumeRatio,2);
	
	float occluderPotential = (normalizedSurfaceArea)/std::pow(((float)numTriangles/1000.0f),2);
	DebugPrint("Min Volume ratio: %f\n", minVolumeRatio);
	DebugPrint("Max Volume ratio: %f\n", maxVolumeRatio);
	DebugPrint("Surface Area: %f\n", meshSurfaceArea);
	return occluderPotential;

}


void MeshTools::SimplifyMesh(std::vector<VertexPosTexNorm> &indexedVertexData, std::vector<uint32_t> &indexData, std::vector<uint32_t> &lodData){
	float threshold = 0.2f;
	size_t targetIndexCount = (size_t)(indexData.size()*threshold);
	float targetError = 1e-2f;
	float lodError = 0.0f;
	lodData.resize(indexData.size());
	lodData.resize(meshopt_simplify(lodData.data(), indexData.data(), indexData.size(),&indexedVertexData[0].vert.x, indexedVertexData.size(), sizeof(VertexPosTexNorm), targetIndexCount, targetError,0,&lodError));
}
