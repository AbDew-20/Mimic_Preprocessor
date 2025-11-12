#pragma once
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <string_view>

struct Vertex{
	DirectX::XMFLOAT3 vert;
	DirectX::XMFLOAT2 texCoord;
	DirectX::XMFLOAT3 normal;

	Vertex() = default;

	Vertex(const DirectX::XMFLOAT3 &v,
		const DirectX::XMFLOAT2 &t,
		const DirectX::XMFLOAT3 &n)
		: vert(v), texCoord(t), normal(n){}
};

namespace FileLoader{
	void ParseObjFile(std::string filePath,
		std::vector<Vertex> &indexedVertexBuffer,
		std::vector<uint32_t> &indexBuffer,
		std::string &materialFile);

	void LoadFileToBuffer(std::string &filePath, std::vector<char> &buffer);

}
