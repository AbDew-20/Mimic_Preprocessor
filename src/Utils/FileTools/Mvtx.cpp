#include <Utils/FileTools/Mvtx.h>
#include <string.h>


void FileTools::MVertex::Deserialize(std::vector<std::byte> &vertexData,std::vector<uint32_t> &indexData, VertexLayout &layout, IReader &reader){
	char magic[4] = {};
	char mvtx[4] = {'M', 'V', 'T', 'X'};
	reader.Read(magic, magicSize);
	if(memcmp(magic, mvtx, magicSize)!=0){
		return;
	}
	reader.Read(&header_, headerSize);
	uint32_t vertDataSize = header_.indexOffset-(headerSize + magicSize);
	vertexData.resize(vertDataSize);
	reader.Read(vertexData.data(), vertDataSize);
	uint32_t indexDataSize = header_.infoOffset-header_.indexOffset;
	indexData.resize(header_.numIndices);
	reader.Read(indexData.data(), indexDataSize);
	layout.attributeData.resize(header_.infoSize/(uint32_t)sizeof(VertexAttributeDesc));
	reader.Read(layout.attributeData.data(), header_.infoSize);
	layout.stride = header_.stride;
}


void FileTools::MVertex::Serialize(const std::vector<std::byte> &vertexData, const std::vector<uint32_t> &indexData, const VertexLayout &layout, IWriter &writer){
	header_.stride = layout.stride;
	header_.numVertices = vertexData.size()/header_.stride;
	header_.numIndices = indexData.size();
	header_.indexOffset = headerSize+magicSize+(uint32_t)vertexData.size();
	header_.infoSize = ((uint32_t)sizeof(VertexAttributeDesc))*layout.attributeData.size();
	header_.infoOffset = header_.indexOffset + sizeof(uint32_t)*header_.numIndices;
	char mvtx[4] = {'M', 'V', 'T', 'X'};
	writer.Write(mvtx, magicSize);
	writer.Write(&header_, headerSize);
	writer.Write(vertexData.data(), vertexData.size());
	writer.Write(indexData.data(), indexData.size()*sizeof(uint32_t));
	writer.Write(layout.attributeData.data(), header_.infoSize);
}
