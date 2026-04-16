#include <Utils/FileTools/Mscn.h>
#include <unordered_set>


void FileTools::MScene::Serialize(const std::vector<OccluderMesh> &occluderMeshData, const std::vector<std::pair<float, size_t>> *pOccluderRankingData, IWriter &writer){
	header_.dataOffset = headerSize+magicSize;
	if(pOccluderRankingData!=nullptr){
	}
	header_.numOccluders = (pOccluderRankingData!=nullptr) ? (uint32_t)pOccluderRankingData->size(): 0UL;
	header_.sceneSize = (uint32_t)occluderMeshData.size()*sizeof(MeshInfo);
	const char mscn[magicSize] = {'M', 'S', 'C', 'N'};
	writer.Write(mscn, magicSize);
	writer.Write(&header_, headerSize);
	std::unordered_set<uint64_t> writtenIndex;
	for(uint32_t i = 0; i<header_.numOccluders; ++i){
		uint64_t occluderIdx = (*pOccluderRankingData)[i].second;
		auto [iter, inserted] = writtenIndex.insert(occluderIdx);
		MeshInfo meshInfo = {};
		FillMeshInfo(meshInfo, occluderMeshData[occluderIdx]);
		writer.Write(&meshInfo, sizeof(MeshInfo));
	}
	for(uint32_t i = 0; i<occluderMeshData.size(); ++i){
		auto [iter, inserted] = writtenIndex.insert(i);
		if(inserted){
			MeshInfo meshInfo = {};
			FillMeshInfo(meshInfo, occluderMeshData[i]);
			writer.Write(&meshInfo, sizeof(MeshInfo));
		}
	}

	
}
void FileTools::MScene::Deserialize(Scene &scene, IReader &reader){
	char magic[4] = {};
	const char mscn[magicSize] = {'M', 'S', 'C', 'N'};
	reader.Read(magic, magicSize);
	if(memcmp(magic, mscn, magicSize)!=0){
		return;
	}
	reader.Read(&header_, headerSize);
	scene.numOccluders = header_.numOccluders;
	uint32_t numElements = header_.sceneSize/sizeof(OccluderMesh);
	scene.meshData.reserve(numElements);
	for(uint32_t i = 0; i<numElements; ++i){
		MeshInfo meshInfo = {};
		reader.Read(&meshInfo, sizeof(MeshInfo));
		scene.meshData.emplace_back(
			std::string(meshInfo.id),
			meshInfo.indexOffset,
			meshInfo.vertexOffset,
			meshInfo.numIndices,
			meshInfo.numIndicesTotal,
			meshInfo.numVertices,
			meshInfo.numVerticesTotal,
			meshInfo.boundingBox,
			meshInfo.occluderScore
		);
	}

}

void FileTools::MScene::FillMeshInfo(MeshInfo &meshInfo, const OccluderMesh &occluderMesh){
	memcpy(&meshInfo,occluderMesh.meshId.substr(0, 36U).c_str(), 36U);
	meshInfo.boundingBox = occluderMesh.boundingBox;
	meshInfo.indexOffset = occluderMesh.indexOffset;
	meshInfo.numIndices = occluderMesh.numIndices;
	meshInfo.numIndicesTotal = occluderMesh.numIndicesTotal;
	meshInfo.numVertices = occluderMesh.numVertices;
	meshInfo.numVerticesTotal = occluderMesh.numVerticesTotal;
	meshInfo.occluderScore = occluderMesh.occluderScore;
	meshInfo.vertexOffset = occluderMesh.vertexOffset;
}
