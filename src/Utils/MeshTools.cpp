#include <Utils/MeshTools.h>
#include <thirdParty/meshoptimizer/meshoptimizer.h>


namespace{
	constexpr uint8_t lineIndexData[24] = {0, 1, 0, 2, 0, 4, 5, 7, 5, 1, 5, 4, 3, 1, 3, 2, 3, 7, 6, 4, 6, 2, 6, 7 };

	struct RefinementParams {
		size_t maxTriangles;
		size_t minTriangles;
		size_t maxVertices;
	};


	void GenerateAABB(meshopt_Meshlet &meshlet, std::vector<uint32_t> &meshletVertices, const VertexPosTexNorm* pIndexedVertexData, AABB &boundingBox){
		DirectX::XMFLOAT3 min(FLT_MAX,FLT_MAX,FLT_MAX);
		DirectX::XMFLOAT3 max(-FLT_MAX,-FLT_MAX,-FLT_MAX);
		for(uint32_t i = 0; i<meshlet.vertex_count; ++i){
			size_t idx = meshletVertices.at(meshlet.vertex_offset+i);
			DirectX::XMFLOAT3 temp = (pIndexedVertexData+idx)->vert;
			
			min = DirectX::XMFLOAT3(std::fminf(temp.x,min.x),std::fminf(temp.y,min.y),std::fminf(temp.z,min.z));
			max = DirectX::XMFLOAT3(std::fmaxf(temp.x,max.x),std::fmaxf(temp.y,max.y),std::fmaxf(temp.z,max.z));
		}
		boundingBox.max = max;
		boundingBox.min = min;
	}
}

void MeshTools::GenerateAABBData(const VertexPosTexNorm *pIndexedVertexData, uint64_t numVertices,  const uint32_t* pIndexData, uint64_t numIndices, size_t limit, std::vector<AABB> *pBoundingBoxData){
	RefinementParams refinement = {8, 8, 8};
	if(limit>32){
		refinement = {limit, limit/4, limit};
	}
	const float fillWeight = 1.0f;
	const size_t maxMeshlets = meshopt_buildMeshletsBound(numIndices, refinement.maxVertices, refinement.minTriangles);
	std::vector<meshopt_Meshlet> meshletData(maxMeshlets);
	std::vector<uint32_t> meshletVertices(numIndices);
	std::vector<uint8_t> meshletTriangles(numIndices);

	size_t meshletCount = meshopt_buildMeshletsSpatial(meshletData.data(),
		meshletVertices.data(),
		meshletTriangles.data(),
		pIndexData,
		numIndices,
		&pIndexedVertexData->vert.x,
		numVertices,
		sizeof(VertexPosTexNorm),
		refinement.maxVertices, refinement.minTriangles,
		refinement.maxTriangles,
		fillWeight);
	AABB meshBoundingBox = {DirectX::XMFLOAT3(-FLT_MAX,-FLT_MAX,-FLT_MAX), DirectX::XMFLOAT3(FLT_MAX,FLT_MAX,FLT_MAX)};
	for(int i = 0; i<meshletCount; ++i){
		AABB boundingBox;
		GenerateAABB(meshletData.at(i), meshletVertices, pIndexedVertexData, boundingBox);
		meshBoundingBox.min = DirectX::XMFLOAT3(std::fminf(meshBoundingBox.min.x, boundingBox.min.x), std::fminf(meshBoundingBox.min.y, boundingBox.min.y), std::fminf(meshBoundingBox.min.z, boundingBox.min.z));
		meshBoundingBox.max = DirectX::XMFLOAT3(std::fmaxf(meshBoundingBox.max.x, boundingBox.max.x), std::fmaxf(meshBoundingBox.max.y, boundingBox.max.y), std::fmaxf(meshBoundingBox.max.z, boundingBox.max.z));
		pBoundingBoxData->push_back(boundingBox);
	}
	pBoundingBoxData->push_back(meshBoundingBox);
	
}

void MeshTools::GenerateAABBWireFrame(const std::vector<AABB> &boundingBoxData, std::vector<VertexPos> *pVertexData, std::vector<uint32_t> *pIndexData){
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
			pVertexData->push_back(vert);
		}

		for(int j = 0; j<24; ++j){
			pIndexData->push_back(offset+(uint32_t)lineIndexData[j]);
		}
	
	}
	
}

void MeshTools::PushBackMeshAABBWireFrame(const AABB &boundingBox,
	std::vector<VertexPos> *pVertexData,
	std::vector<uint32_t> *pIndexData)
{
	const uint8_t xMask = 0b00000001;
	const uint8_t yMask = 0b00000010;
	const uint8_t zMask = 0b00000100;
	const uint32_t offset = pVertexData->size();
	for(uint8_t j = 0; j<8; ++j){
		VertexPos vert;
		vert.vert.x = (j&xMask) ? boundingBox.max.x : boundingBox.min.x;
		vert.vert.y = (j&yMask) ? boundingBox.max.y : boundingBox.min.y;
		vert.vert.z = (j&zMask) ? boundingBox.max.z : boundingBox.min.z;
		pVertexData->push_back(vert);
	}
	for(int j = 0; j<24; ++j){
		pIndexData->push_back(offset+(uint32_t)lineIndexData[j]);
	}
}

float MeshTools::GetOccluderPotential(const std::vector<AABB> &minBoundingBoxData, const std::vector<AABB> &maxBoundingBoxData, size_t numTriangles){
	float minInteriorVolume = 0.0f;
	float maxInteriorVolume = 0.0f;
	float meshSurfaceArea = 0.0f;
	AABB meshBoundingBox = maxBoundingBoxData.back();
	float meshVolume = meshBoundingBox.GetAABBVolume();
	meshVolume = (meshVolume>0) ? meshVolume : 1.0f;
	for(size_t i = 0; i<minBoundingBoxData.size()-1; ++i){
		minInteriorVolume += minBoundingBoxData.at(i).GetAABBVolume();
	}
	meshSurfaceArea = meshBoundingBox.GetAABBSurfaceArea();
	float minVolumeRatio = minInteriorVolume/meshVolume;
	for(size_t i = 0; i<maxBoundingBoxData.size()-1; ++i){
		maxInteriorVolume += maxBoundingBoxData.at(i).GetAABBVolume();
	}
	float maxVolumeRatio = maxInteriorVolume/meshVolume;

	float linearMinVolumeRatio = (minVolumeRatio<1.0f) ? 1.0f/minVolumeRatio : minVolumeRatio;
	float linearMaxVolumeRatio = (maxVolumeRatio<1.0f) ? 1.0f/maxVolumeRatio : maxVolumeRatio;

	float normalizedLinearVolumeRatio = (linearMaxVolumeRatio+linearMinVolumeRatio)/2.0f;

	float normalizedSurfaceArea = meshSurfaceArea/std::pow(normalizedLinearVolumeRatio,2.5f);
	
	float occluderPotential = (normalizedSurfaceArea)/std::pow(((float)numTriangles/1000.0f),1.5f);
	//DebugPrint("Min Volume ratio: %f\n", minVolumeRatio);
	//DebugPrint("Max Volume ratio: %f\n", maxVolumeRatio);
	//DebugPrint("Surface Area: %f\n", meshSurfaceArea);
	return occluderPotential;

}


void MeshTools::SimplifyMesh(const VertexPosTexNorm *pIndexedVertexData, size_t numVertices, const uint32_t *pIndexData, size_t numIndices, std::vector<uint32_t> *pLodData){
	float threshold = 0.2f;
	size_t targetIndexCount = (size_t)(numIndices*threshold);
	float targetError = 1e-2f;
	float lodError = 0.0f;
	pLodData->resize(numIndices);
	pLodData->resize(meshopt_simplify(pLodData->data(), pIndexData, numIndices,&pIndexedVertexData->vert.x, numVertices, sizeof(VertexPosTexNorm), targetIndexCount, targetError,0,&lodError));
}

AABB MeshTools::GetAABB(const VertexPosTexNorm *pIndexedVertexData, uint64_t numVertices){
	AABB boundingBox;
	DirectX::XMFLOAT3 min(FLT_MAX,FLT_MAX,FLT_MAX);
	DirectX::XMFLOAT3 max(-FLT_MAX,-FLT_MAX,-FLT_MAX);
	for(uint32_t i = 0; i<numVertices; ++i){
		DirectX::XMFLOAT3 temp = (pIndexedVertexData+i)->vert;
		
		min = DirectX::XMFLOAT3(std::fminf(temp.x,min.x),std::fminf(temp.y,min.y),std::fminf(temp.z,min.z));
		max = DirectX::XMFLOAT3(std::fmaxf(temp.x,max.x),std::fmaxf(temp.y,max.y),std::fmaxf(temp.z,max.z));
	}
	boundingBox.max = max;
	boundingBox.min = min;
	return boundingBox;
}

