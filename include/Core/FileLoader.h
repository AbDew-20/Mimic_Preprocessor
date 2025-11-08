#pragma once
#include <DirectXMath.h>
#include <vector>
#include <string>

enum class HeaderCode{
	vertex,
	normal,
	texture,
	face,
	group,
	mtlFile,
	material,
	undef
};
struct VertexData{
	DirectX::XMFLOAT3 vert;
	DirectX::XMFLOAT2 texCoord;
	DirectX::XMFLOAT3 normal;
};

class FileLoader{
public:
	FileLoader(std::string filePath, std::vector<DirectX::XMFLOAT3> &vertexBuffer,std::vector<DirectX::XMFLOAT2> &texCoordBuffer,std::vector<DirectX::XMFLOAT3> &vertNormalBuffer,std::vector<VertexData> &interleavedBuffer);
	void ParseObjFile();
private:
	HeaderCode HashString(const std::string &header);
	bool ParseString(std::string string, const char delim, std::vector<std::tuple<size_t,size_t>> &offsets);
	bool GetToken(const std::string &line, const std::vector<std::tuple<size_t,size_t>> &tokenOffsets,const size_t tokenIdx, std::string &output);
	VertexData GetVertexData(std::string &vertToken);
	std::string filePath_;
	std::string materialFile_;
	std::vector<DirectX::XMFLOAT3> &vertexBuffer_;
	std::vector<DirectX::XMFLOAT2> &texCoordBuffer_;
	std::vector<DirectX::XMFLOAT3> &vertNormalBuffer_;
	std::vector<VertexData> &interleavedBuffer_;



};
