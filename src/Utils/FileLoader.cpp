#include <Utils/FileLoader.h>
#include <Core/PCH.h>
#include <fstream>
#include <charconv>
#include <thirdParty/meshoptimizer/meshoptimizer.h>




namespace{
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

	HeaderCode HashString(const std::string_view header){
		if(header=="mtllib") return HeaderCode::mtlFile;
		if(header=="v") return HeaderCode::vertex;
		if(header=="vt") return HeaderCode::texture;
		if(header=="vn") return HeaderCode::normal;
		if(header=="g") return HeaderCode::group;
		if(header=="usemtl") return HeaderCode::material;
		if(header=="f") return HeaderCode::face;
		return HeaderCode::undef;
	}

	void GetLines(std::vector<char> &buffer, std::vector<std::string_view> &lines){
		const char *start = buffer.data();
		const char *end = start+buffer.size();
		const char *lineStart = start;

		while(lineStart<end){
			const char *newLine = static_cast<const char *>(memchr(lineStart, '\n', end-lineStart));
			if(!newLine) newLine = end;

			size_t lineLength = newLine-lineStart;
			if(lineLength>0&&lineStart[lineLength-1]=='\r'){
				--lineLength;  // trim '\r'
			}
			lines.emplace_back(lineStart, lineLength);
			lineStart = newLine+(newLine<end ? 1 : 0);
		}


	}

	bool ParseString(std::string_view string, const char delim, std::vector<std::string_view> &tokenList){
		size_t runningOffset = 0;
		while(string.size()>runningOffset){
			size_t offset = string.find_first_of(delim, runningOffset);
			if(offset==std::string::npos){
				offset = string.size();
			}
			if(offset!=runningOffset) tokenList.emplace_back(string.substr(runningOffset, offset-runningOffset));
			runningOffset = offset+1;
		}
		return true;

	}

	void GetVertexData(std::vector<std::string_view> &vertTokenList,
		std::vector<DirectX::XMFLOAT3> &vertPosBuffer,
		std::vector<DirectX::XMFLOAT2> &texCoordBuffer,
		std::vector<DirectX::XMFLOAT3> &vertNormalBuffer,
		VertexPosTexNorm &vertData){
		vertData.vert = {0.0f,0.0f,0.0f};
		vertData.texCoord = {0.0f,0.0f};
		vertData.normal = {0.0f,0.0f,0.0f};
		std::string_view token;
		token = vertTokenList.at(0);
		int vertIdx = 0;
		std::from_chars(token.data(), token.data()+token.size(), vertIdx);
		if(vertIdx<0){
			vertIdx += vertPosBuffer.size();
		}
		else{
			vertIdx--;
		}
		vertData.vert = vertPosBuffer.at(vertIdx);

		if(texCoordBuffer.size()>0){
			token = vertTokenList.at(1);
			int texCoordIdx = 0;
			std::from_chars(token.data(), token.data()+token.size(), texCoordIdx);
			if(texCoordIdx<0){
				texCoordIdx += texCoordBuffer.size();
			}
			else{
				texCoordIdx--;
			}
			vertData.texCoord = texCoordBuffer.at(texCoordIdx);
		}


		size_t vertTokenIdx = 2;
		if(vertNormalBuffer.size()>0){
			if(texCoordBuffer.size()==0) vertTokenIdx = 1;
			token = vertTokenList.at(vertTokenIdx);
			int normIdx = 0;
			std::from_chars(token.data(), token.data()+token.size(), normIdx);
			if(normIdx<0){
				normIdx += vertNormalBuffer.size();
			}
			else{
				normIdx--;
			}
			vertData.normal = vertNormalBuffer.at(normIdx);
		}
	}

	void GenerateIndexBuffer(std::vector<VertexPosTexNorm> &interleavedBuffer, std::vector<VertexPosTexNorm> &indexedInterleavedBuffer, std::vector<uint32_t> &indexBuffer){
		size_t numIndices = interleavedBuffer.size();
		std::vector<uint32_t> remap(numIndices);
		size_t numVertices = meshopt_generateVertexRemap(remap.data(), nullptr, numIndices, interleavedBuffer.data(), numIndices, sizeof(VertexPosTexNorm));
		indexedInterleavedBuffer.resize(numVertices);
		indexBuffer.resize(numIndices);
		meshopt_remapVertexBuffer(indexedInterleavedBuffer.data(), interleavedBuffer.data(), numIndices, sizeof(VertexPosTexNorm), remap.data());
		meshopt_remapIndexBuffer(indexBuffer.data(), nullptr, numIndices, remap.data());
	}
}



void FileLoader::ParseObjFile(std::string filePath,
		std::vector<VertexPosTexNorm> &indexedVertexBuffer,
		std::vector<uint32_t> &indexBuffer,
		std::string& materialFile){

	std::vector<DirectX::XMFLOAT3> vertPosBuffer;
	std::vector<DirectX::XMFLOAT2> texCoordBuffer;
	std::vector<DirectX::XMFLOAT3> vertNormalBuffer;
	std::vector<VertexPosTexNorm> interleavedBuffer;

	vertPosBuffer.reserve(500);
	texCoordBuffer.reserve(500);
	vertNormalBuffer.reserve(500);
	interleavedBuffer.reserve(500);

	std::vector<std::string_view> lines;
	std::vector<char> buffer;
	LoadFileToBuffer(filePath, buffer);
	GetLines(buffer, lines);

	//scratch buffers for various types
	std::string line;
	std::string_view header;
	std::string_view token;
	VertexPosTexNorm vertData = {DirectX::XMFLOAT3(),DirectX::XMFLOAT2(),DirectX::XMFLOAT3()};
	std::vector<std::string_view> vertTokenList;
	vertTokenList.reserve(3);
	std::vector<std::string_view> tokenList;
	tokenList.reserve(20);


	int numLine = 0;
	for(auto lineSv:lines){
		numLine++;
		tokenList.clear();
		if(!(lineSv.size()>1)){
			continue;
		}
		ParseString(lineSv, ' ', tokenList);
		header = tokenList.at(0);
		HeaderCode hCode = HashString(header);
		switch(hCode){
		case HeaderCode::mtlFile:
		{
			materialFile = tokenList.at(1);
		}
			break;
		case HeaderCode::face:
		{
			size_t numTriangles = tokenList.size()-3;
			
			token = tokenList.at(1);
			vertTokenList.clear();
			ParseString(token, '/', vertTokenList);
			VertexPosTexNorm vert0;
			GetVertexData(vertTokenList, vertPosBuffer, texCoordBuffer, vertNormalBuffer, vert0);


			for(size_t i = 0; i<numTriangles; ++i){
				interleavedBuffer.push_back(vert0);
				for(size_t j = 0; j<2; ++j){
					token = tokenList.at(i+2+j);
					vertTokenList.clear();
					ParseString(token, '/', vertTokenList);
					GetVertexData(vertTokenList,vertPosBuffer,texCoordBuffer,vertNormalBuffer, vertData);
					interleavedBuffer.push_back(vertData);
				}
					
			
			}


		
		}

		break;
		case HeaderCode::vertex:
		{
			token = tokenList.at(1);
			float vx = 0.0f;
			std::from_chars(token.data(), token.data()+token.size(), vx);
			token = tokenList.at(2);
			float vy = 0.0f;
			std::from_chars(token.data(), token.data()+token.size(), vy);
			token = tokenList.at(3);
			float vz = 0.0f;
			std::from_chars(token.data(), token.data()+token.size(), vz);
			vertPosBuffer.emplace_back(vx,vy,vz);
		}
		break;
		case HeaderCode::texture:
		{
			token = tokenList.at(1);
			float u = 0.0f;
			std::from_chars(token.data(), token.data()+token.size(), u);
			token = tokenList.at(2);
			float v = 0.0f;
			std::from_chars(token.data(), token.data()+token.size(), v);
			texCoordBuffer.emplace_back(u,v);
		}
		break;

		case HeaderCode::normal:
		{
			token = tokenList.at(1);
			float nx = 0.0f;
			std::from_chars(token.data(), token.data()+token.size(), nx);
			token = tokenList.at(2);
			float ny = 0.0f;
			std::from_chars(token.data(), token.data()+token.size(), ny);
			token = tokenList.at(3);
			float nz = 0.0f;
			std::from_chars(token.data(), token.data()+token.size(), nz);
			vertNormalBuffer.emplace_back(nx,ny,nz);
		}
		break;
		case HeaderCode::undef:
			continue;
			break;

		}

	}

	GenerateIndexBuffer(interleavedBuffer, indexedVertexBuffer, indexBuffer);

}


void FileLoader::LoadFileToBuffer(std::string &filePath, std::vector<char> &buffer){
	std::ifstream readFile(filePath.data(), std::ios::binary|std::ios::ate);
	assert(readFile.is_open()&&"Error opening file");
	size_t fileSize = readFile.tellg();
	buffer.assign(fileSize, '\0');
	readFile.seekg(0);
	readFile.read(buffer.data(), fileSize);
	readFile.close();
}
