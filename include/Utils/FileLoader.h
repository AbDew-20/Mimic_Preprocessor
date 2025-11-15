#pragma once
#include <DirectXMath.h>
#include <vector>
#include <string>
#include <string_view>
#include <Core/VertexTypes.h>


namespace FileLoader{
	void ParseObjFile(std::string filePath,
		std::vector<VertexPosTexNorm> &indexedVertexBuffer,
		std::vector<uint32_t> &indexBuffer,
		std::string &materialFile);

	void LoadFileToBuffer(std::string &filePath, std::vector<char> &buffer);

}
