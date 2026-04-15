#pragma once
#include <vector>
#include <Core/IO/IReader.h>
#include <Core/IO/IWriter.h>
#include <Core/VertexTypes.h>
#include <cstddef>

namespace FileTools{

	class MVertex{
		struct Header{
			uint32_t infoOffset = 0;
			uint32_t infoSize = 0;
			uint32_t stride = 0;
			uint32_t numVertices = 0;
			uint32_t indexOffset = 0;
			uint32_t numIndices = 0;
		};
	public:
		MVertex() = default;
		void Deserialize(std::vector<std::byte> &vertexData, std::vector<uint32_t> &indexData, VertexLayout &layout, IReader &reader);
		void Serialize(const std::vector<std::byte> &vertexData, const std::vector<uint32_t> &indexData, const VertexLayout &layout, IWriter &writer);
		static constexpr uint32_t headerSize = (uint32_t)(sizeof(Header));
		static constexpr uint32_t magicSize = 4U;
	protected:
	private:
		Header header_;
	};
}