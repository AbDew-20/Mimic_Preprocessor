#include <Core/FileLoader.h>
#include <Core/PCH.h>
#include <fstream>



FileLoader::FileLoader(std::string filePath,
	std::vector<DirectX::XMFLOAT3> &vertexBuffer,
	std::vector<DirectX::XMFLOAT2> &texCoordBuffer,
	std::vector<DirectX::XMFLOAT3> &vertNormalBuffer,
	std::vector<VertexData> &interleavedBuffer):
	filePath_(filePath),
	vertexBuffer_(vertexBuffer),
	texCoordBuffer_(texCoordBuffer),
	interleavedBuffer_(interleavedBuffer),
	vertNormalBuffer_(vertNormalBuffer)
{
}

void FileLoader::ParseObjFile(){
	std::ifstream readFile(filePath_.c_str());
	assert(readFile.is_open() &&"Error opening file");
	std::string line;
	size_t lineNumber = 0;
	while(std::getline(readFile, line)){
		if(!(line.size()>1)){
			continue;
		}
		std::vector<std::tuple<size_t,size_t>> tokenOffsets = {};
		ParseString(line, ' ', tokenOffsets);
		std::string header = "";
		GetToken(line, tokenOffsets, 0, header);
		HeaderCode hCode = HashString(header);
		
		switch(hCode){
		case HeaderCode::mtlFile:
			GetToken(line, tokenOffsets, 1, materialFile_);
			break;
		case HeaderCode::face:
		{
			for(int i = 1; i<4; ++i){
				std::string vertToken = "";
				GetToken(line, tokenOffsets, (size_t)i, vertToken);
				interleavedBuffer_.push_back(GetVertexData(vertToken));
			}
			if(tokenOffsets.size()>4){																//Triangulating Quad
				interleavedBuffer_.push_back(interleavedBuffer_.at(interleavedBuffer_.size()-1-2));
				interleavedBuffer_.push_back(interleavedBuffer_.at(interleavedBuffer_.size()-1-1));
				std::string vertToken = "";
				GetToken(line, tokenOffsets, 4, vertToken);
				interleavedBuffer_.push_back(GetVertexData(vertToken));
			}
		
		}

		break;
		case HeaderCode::vertex:
		{
			std::string token="";
			GetToken(line, tokenOffsets, 1, token);
			float vx = std::stof(token, nullptr);
			GetToken(line, tokenOffsets, 2, token);
			float vy= std::stof(token, nullptr);
			GetToken(line, tokenOffsets, 3, token);
			float vz = std::stof(token, nullptr);
			vertexBuffer_.push_back(DirectX::XMFLOAT3(vx, vy, vz));
		}
		break;
		case HeaderCode::texture:
		{
			std::string token="";
			GetToken(line, tokenOffsets, 1, token);
			float u = std::stof(token, nullptr);
			GetToken(line, tokenOffsets, 2, token);
			float v= std::stof(token, nullptr);
			texCoordBuffer_.push_back(DirectX::XMFLOAT2(u, v));
		}
		break;

		case HeaderCode::normal:
		{
			std::string token="";
			GetToken(line, tokenOffsets, 1, token);
			float nx = std::stof(token, nullptr);
			GetToken(line, tokenOffsets, 2, token);
			float ny= std::stof(token, nullptr);
			GetToken(line, tokenOffsets, 3, token);
			float nz = std::stof(token, nullptr);
			vertNormalBuffer_.push_back(DirectX::XMFLOAT3(nx, ny,nz));
		}
		break;
		case HeaderCode::undef:
			continue;
			break;

		}

		lineNumber++;
	}
	readFile.close();
}

HeaderCode FileLoader::HashString(const std::string &header){
	if(header=="mtllib") return HeaderCode::mtlFile;
	if(header=="v") return HeaderCode::vertex;
	if(header=="vt") return HeaderCode::texture;
	if(header=="vn") return HeaderCode::normal;
	if(header=="g") return HeaderCode::group;
	if(header=="usemtl") return HeaderCode::material;
	if(header=="f") return HeaderCode::face;
	return HeaderCode::undef;
}

bool FileLoader::ParseString(std::string string, const char delim, std::vector<std::tuple<size_t,size_t>> &offsets){
	size_t runningOffset = 0;
	while(string.size()>0){
		size_t offset = string.find_first_of(delim);
		if(offset==std::string::npos){
			offset = string.size();
		}
		if(offset>0) offsets.push_back(std::tuple<size_t,size_t>(runningOffset,runningOffset+offset));
		string.erase(0, offset+1);
		runningOffset += (offset+1);
	}
	return true;

}
bool FileLoader::GetToken(const std::string &line, const std::vector<std::tuple<size_t,size_t>> &tokenOffsets, const size_t tokenIdx, std::string &output){
	assert(tokenIdx<tokenOffsets.size()&& "Accessing out of bound token");
	output = line.substr(std::get<0>(tokenOffsets.at(tokenIdx)), std::get<1>(tokenOffsets.at(tokenIdx)));
	return true;
}

VertexData FileLoader::GetVertexData(std::string &vertToken){
	std::vector<std::tuple<size_t,size_t>> vertTokenOffsets = {};
	ParseString(vertToken, '/', vertTokenOffsets);
	DirectX::XMFLOAT3 vertPos = {0.0f,0.0f,0.0f};
	DirectX::XMFLOAT2 texCoord= {0.0f,0.0f};
	DirectX::XMFLOAT3 vertNorm= {0.0f,0.0f,0.0f};
	std::string token="";
	GetToken(vertToken, vertTokenOffsets, 0, token);
	int vertIdx = std::stoi(token, nullptr);
	if(vertIdx<0){
		vertIdx += vertexBuffer_.size();
	}
	else{
		vertIdx--;
	}
	vertPos = vertexBuffer_.at(vertIdx);

	if(texCoordBuffer_.size()>0){
		GetToken(vertToken, vertTokenOffsets, 1, token);
		int texCoordIdx = std::stoi(token, nullptr);
		if(texCoordIdx<0){
			texCoordIdx += texCoordBuffer_.size();
		}
		else{
			texCoordIdx--;
		}
		texCoord = texCoordBuffer_.at(texCoordIdx);
	}


	if(vertNormalBuffer_.size()>0){
		GetToken(vertToken, vertTokenOffsets, 2, token);
		int normIdx = std::stoi(token, nullptr);
		if(normIdx<0){
			normIdx += vertNormalBuffer_.size();	
		}else{
			normIdx--;
		}
		vertNorm = vertNormalBuffer_.at(normIdx);
	}
	return {vertPos,texCoord,vertNorm};
}
