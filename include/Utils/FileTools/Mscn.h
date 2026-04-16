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
			float occluderScore = 0.0f;
			uint64_t indexOffset = 0ULL;
			uint32_t numIndices = 0UL;
			uint32_t numIndicesTotal = 0UL;
			uint64_t vertexOffset = 0ULL;
			uint32_t numVertices = 0UL;
			uint32_t numVerticesTotal = 0UL;
			AABB boundingBox;
		};
		struct Header{
			uint32_t dataOffset = 0UL;
			uint32_t sceneSize = 0UL;
			uint32_t numOccluders = 0UL;
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