#pragma once
#include <vector>
#include <Core/MeshTypes.h>
#include <Core/AABB.h>
#include <Core/IO/IWriter.h>
#include <Core/IO/IReader.h>
#include <Core/Scene.h>



namespace FileTools{

	class MScene{
		struct MeshInfo{
			char id[36];
			float occluderScore;
			uint64_t indexOffset;
			uint32_t numIndices;
			uint32_t numIndicesTotal;
			uint64_t vertexOffset;
			uint32_t numVertices;
			uint32_t numVerticesTotal;
			AABB boundingBox;
		};
		struct Header{
			uint32_t dataOffset;
			uint32_t sceneSize;
			uint32_t numOccluders;
		};

	public:
		MScene() = default;
		void Serialize(const std::vector<OccluderMesh> &occluderMeshData, const std::vector<std::pair<float, size_t>> *pOccluderRankingData, IWriter &writer);
		void Deserialize(Scene &scene, IReader &reader);
		static constexpr uint32_t headerSize = (uint32_t)sizeof(Header);
		static constexpr uint32_t magicSize = 4UL;
	protected:
	private:
		void FillMeshInfo(MeshInfo &meshInfo, const OccluderMesh &occluderMesh);
		Header header_;
	
	};
}