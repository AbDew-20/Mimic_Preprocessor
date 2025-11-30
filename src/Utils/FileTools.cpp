#include <Utils/FileTools.h>
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
		object,
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
		if(header=="o") return HeaderCode::object;
		if(header=="usemtl") return HeaderCode::material;
		if(header=="f") return HeaderCode::face;
		return HeaderCode::undef;
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

}


void FileTools::LoadFileToBuffer(const std::string &filePath, std::vector<char> *pBuffer){
	std::ifstream readFile(filePath.data(), std::ios::binary|std::ios::ate);
	assert(readFile.is_open()&&"Error opening file");
	size_t fileSize = readFile.tellg();
	pBuffer->assign(fileSize, '\0');
	readFile.seekg(0);
	readFile.read(pBuffer->data(), fileSize);
	readFile.close();
}



FileTools::Obj::Obj(const std::string &fileName) :
	fileName_(fileName){
	SYSTEM_INFO sysInfo = {};
	::GetSystemInfo(&sysInfo);
	allocGranularity_ = sysInfo.dwAllocationGranularity;
	pageSize_ = allocGranularity_*512;
}


void FileTools::Obj::MapFile(){
	hFile_=::CreateFileA(fileName_.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
	assert(!(hFile_==INVALID_HANDLE_VALUE)&&"Failed to create file handle");
	LARGE_INTEGER fileSize = {};
	GetFileSizeEx(hFile_, &fileSize);
	fileSize_ = fileSize.QuadPart;


	hMap_ = ::CreateFileMappingA(hFile_, NULL, PAGE_READONLY, 0, 0, NULL);
	assert(!(hMap_==INVALID_HANDLE_VALUE)&&"Failed to map file");

}

void FileTools::Obj::SetView(SIZE_T offset){
	DWORD offsetHi = (DWORD)(offset>>32U);
	DWORD offsetLo = (DWORD)(offset&0xFFFFFFFFUL);
	uint64_t bytes = offset+pageSize_>fileSize_ ? 0: pageSize_;

	if(pView_){
		::UnmapViewOfFile(pView_);
	}
	
	pView_ = ::MapViewOfFile(hMap_, FILE_MAP_READ, offsetHi, offsetLo, bytes);
	assert(!(pView_==NULL)&&"Failed to load view from map");
	viewSize_ = (fileSize_-offset<pageSize_)? fileSize_-offset: pageSize_;
}

void FileTools::Obj::ParseLines(size_t startOffset, size_t *pOutStartOffset, std::vector<std::string_view> *pLines) const{
	const char *end = static_cast<char *>(pView_)+viewSize_;
	const char *lastLine = end;
	const char *startView = static_cast<char *>(pView_);
	const char *start = startView+startOffset;
	if(viewSize_==pageSize_){
		while(lastLine--){
			if((*lastLine)=='\n'){
				break;
			}
		}
	}
	*pOutStartOffset = allocGranularity_-(end-lastLine)+1;

	const char *lineStart = start;
	while(lineStart<lastLine){
		const char *newLine = static_cast<const char *>(memchr(lineStart,'\n', lastLine-lineStart));
		if(!newLine) newLine = lastLine;
		size_t lineLength = newLine-lineStart;
		if(lineLength>0&&lineStart[lineLength-1]=='\r'){
			--lineLength;
		}
		std::string_view line(lineStart, lineLength);
		pLines->push_back(line);
		lineStart = newLine+(newLine<lastLine ? 1 : 0);
	}
}
void FileTools::Obj::ParseObjFile(
	std::vector<VertexPosTexNorm> *pIndexedVertexBuffer,
	std::vector<uint32_t> *pIndexBuffer,
	std::string *pMaterialFile,
	std::vector<SubMesh> *pMeshOffsetData){

	std::vector<DirectX::XMFLOAT3> vertPosBuffer;
	std::vector<DirectX::XMFLOAT2> texCoordBuffer;
	std::vector<DirectX::XMFLOAT3> vertNormalBuffer;
	std::vector<VertexPosTexNorm> interleavedBuffer;


	vertPosBuffer.reserve(500);
	texCoordBuffer.reserve(500);
	vertNormalBuffer.reserve(500);
	interleavedBuffer.reserve(500);

	std::vector<std::string_view> lines;

	//scratch buffers for various types
	std::string_view header;
	std::string_view token;
	VertexPosTexNorm vertData = {DirectX::XMFLOAT3(),DirectX::XMFLOAT2(),DirectX::XMFLOAT3()};
	std::vector<std::string_view> vertTokenList;
	vertTokenList.reserve(3);
	std::vector<std::string_view> tokenList;
	tokenList.reserve(20);
	std::string groupName;
	std::string objectName;
	std::string mtlName;
	std::vector<SubMesh> alphaTestedMeshData;
	alphaTestedMeshData.reserve(10);
	std::vector<VertexPosTexNorm> alphaTestedVertexData;
	std::vector<uint32_t> alphaTestedIndexData;
	alphaTestedVertexData.reserve(500);
	alphaTestedIndexData.reserve(500);

	size_t numPages = (fileSize_/(pageSize_-allocGranularity_))+((fileSize_%(pageSize_-allocGranularity_)==0)? 0: 1);
	numPages = (numPages==0) ? 1 : numPages;
	uint64_t startOffset = 0;
	uint64_t viewOffset =0;
	for(size_t pageId = 0; pageId<numPages; pageId++){
		viewOffset += pageSize_-allocGranularity_;
		if(pageId==0){
			viewOffset = 0;
		}
		SetView(viewOffset);
		lines.clear();
		Obj::ParseLines(startOffset, &startOffset, &lines);

		for(auto lineSv:lines){
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
				*pMaterialFile = tokenList.at(1);
			}
			break;
			case HeaderCode::face:
			{
				size_t listSize = tokenList.size();
				size_t numTriangles = listSize-3;
				token = tokenList.at(1);
				vertTokenList.clear();
				ParseString(token, '/', vertTokenList);
				VertexPosTexNorm vert0;
				Obj::LoadVertexData(vertTokenList, vertPosBuffer,
					texCoordBuffer, vertNormalBuffer, &vert0);


				for(size_t i = 0; i<numTriangles; ++i){
					interleavedBuffer.push_back(vert0);
					for(size_t j = 0; j<2; ++j){
						token = tokenList.at((i+2+j));
						vertTokenList.clear();
						ParseString(token, '/', vertTokenList);
						Obj::LoadVertexData(vertTokenList, vertPosBuffer,
							texCoordBuffer, vertNormalBuffer, &vertData);
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
				vertPosBuffer.emplace_back(vx, vy, vz);
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
				texCoordBuffer.emplace_back(u, v);
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
				vertNormalBuffer.emplace_back(nx, ny, nz);
			}
			break;
			case HeaderCode::group:
			{
				if(!interleavedBuffer.empty()){
					std::string meshName;
					meshName.append(objectName);
					meshName.append(groupName);
					meshName.append(mtlName);
					bool alphaTested = false; //TODO: check alphatesting
					pMeshOffsetData->push_back({objectName,groupName, mtlName, pIndexBuffer->size(), interleavedBuffer.size(), pIndexedVertexBuffer->size(), 0, alphaTested});
					Obj::GenerateIndexBuffer(interleavedBuffer, pIndexedVertexBuffer, pIndexBuffer);
					pMeshOffsetData->back().numVertices = pIndexedVertexBuffer->size()-pMeshOffsetData->back().vertexOffset;
					interleavedBuffer.clear();
				}
				if(!alphaTestedMeshData.empty()&&objectName==""){
					Obj::PushBackData(alphaTestedMeshData, alphaTestedVertexData, alphaTestedIndexData, pIndexedVertexBuffer, pIndexBuffer, pMeshOffsetData);
					alphaTestedMeshData.clear();
					alphaTestedIndexData.clear();
					alphaTestedVertexData.clear();
				}
				groupName = tokenList.at(1);

			}
			break;
			case HeaderCode::object:
			{
				if(!interleavedBuffer.empty()){
					std::string meshName;
					meshName.append(objectName);
					meshName.append(groupName);
					meshName.append(mtlName);
					bool alphaTested = false; //TODO: check alphatesting
					pMeshOffsetData->push_back({objectName,groupName, mtlName, pIndexBuffer->size(), interleavedBuffer.size(), pIndexedVertexBuffer->size(), 0, alphaTested});
					Obj::GenerateIndexBuffer(interleavedBuffer, pIndexedVertexBuffer, pIndexBuffer);
					pMeshOffsetData->back().numVertices = pIndexedVertexBuffer->size()-pMeshOffsetData->back().vertexOffset;
					interleavedBuffer.clear();
				}
				if(!alphaTestedMeshData.empty()){
					Obj::PushBackData(alphaTestedMeshData, alphaTestedVertexData, alphaTestedIndexData, pIndexedVertexBuffer, pIndexBuffer, pMeshOffsetData);
					alphaTestedMeshData.clear();
					alphaTestedIndexData.clear();
					alphaTestedVertexData.clear();
				}
				objectName = tokenList.at(1);
			
			}
			break;
			case HeaderCode::material:
			{
				if(!interleavedBuffer.empty()){
					std::string meshName;
					meshName.append(objectName);
					meshName.append(groupName);
					bool alphaTested = false; //TODO: Check against material
					if(alphaTested && meshName!=""){
						alphaTestedMeshData.push_back({objectName, groupName, mtlName, alphaTestedIndexData.size(), interleavedBuffer.size(), alphaTestedVertexData.size(), 0, alphaTested});
						Obj::GenerateIndexBuffer(interleavedBuffer, &alphaTestedVertexData, &alphaTestedIndexData);
						alphaTestedMeshData.back().numVertices = alphaTestedIndexData.size()-alphaTestedMeshData.back().vertexOffset;
						interleavedBuffer.clear();
					}
					else{
						pMeshOffsetData->push_back({objectName,groupName, mtlName, pIndexBuffer->size(), interleavedBuffer.size(), pIndexedVertexBuffer->size(), 0, alphaTested});
						Obj::GenerateIndexBuffer(interleavedBuffer, pIndexedVertexBuffer, pIndexBuffer);
						pMeshOffsetData->back().numVertices = pIndexedVertexBuffer->size()-pMeshOffsetData->back().vertexOffset;
						interleavedBuffer.clear();
					}
				}
				mtlName = tokenList.at(1);

			}
			break;
			case HeaderCode::undef:
				continue;
				break;

			}

		}
	}
	bool alphaTested = false; //TODO: check alphaTesting
	pMeshOffsetData->push_back({objectName,groupName, mtlName, pIndexBuffer->size(), interleavedBuffer.size(), pIndexedVertexBuffer->size(), 0, alphaTested});
	Obj::GenerateIndexBuffer(interleavedBuffer, pIndexedVertexBuffer, pIndexBuffer);
	pMeshOffsetData->back().numVertices = pIndexedVertexBuffer->size()-pMeshOffsetData->back().vertexOffset;
}


void FileTools::Obj::GenerateIndexBuffer(const std::vector<VertexPosTexNorm> &interleavedBuffer, std::vector<VertexPosTexNorm> *pIndexedInterleavedBuffer, std::vector<uint32_t> *pIndexBuffer) const{
	size_t numIndices = interleavedBuffer.size();
	std::vector<uint32_t> remap(numIndices);
	size_t numVertices = meshopt_generateVertexRemap(remap.data(), nullptr, numIndices, interleavedBuffer.data(), numIndices, sizeof(VertexPosTexNorm));
	size_t vertexBufferOffset = pIndexedInterleavedBuffer->size();
	size_t indexBufferOffset = pIndexBuffer->size();
	pIndexedInterleavedBuffer->resize(vertexBufferOffset+ numVertices);
	pIndexBuffer->resize(indexBufferOffset+ numIndices);
	meshopt_remapVertexBuffer(pIndexedInterleavedBuffer->data() +vertexBufferOffset, interleavedBuffer.data(), numIndices, sizeof(VertexPosTexNorm), remap.data());
	meshopt_remapIndexBuffer(pIndexBuffer->data()+indexBufferOffset, nullptr, numIndices, remap.data());
	for(auto iter = pIndexBuffer->begin()+indexBufferOffset; iter!=pIndexBuffer->end(); ++iter){
		*iter += (uint32_t)vertexBufferOffset;
	}
}
void FileTools::Obj::LoadVertexData(
	const std::vector<std::string_view> &vertTokenList,
	const std::vector<DirectX::XMFLOAT3> &vertPosBuffer,
	const std::vector<DirectX::XMFLOAT2> &texCoordBuffer,
	const std::vector<DirectX::XMFLOAT3> &vertNormalBuffer,
	VertexPosTexNorm *pVertData) const{
	pVertData->vert = {0.0f,0.0f,0.0f};
	pVertData->texCoord = {0.0f,0.0f};
	pVertData->normal = {0.0f,0.0f,0.0f};
	std::string_view token;
	token = vertTokenList.at(0);
	int64_t vertIdx = 0;
	std::from_chars(token.data(), token.data()+token.size(), vertIdx);
	if(vertIdx<0){
		vertIdx += vertPosBuffer.size();
	}
	else{
		vertIdx--;
	}
	pVertData->vert = vertPosBuffer.at(vertIdx);

	if(texCoordBuffer.size()>0){
		token = vertTokenList.at(1);
		int64_t texCoordIdx = 0;
		std::from_chars(token.data(), token.data()+token.size(), texCoordIdx);
		if(texCoordIdx<0){
			texCoordIdx += texCoordBuffer.size();
		}
		else{
			texCoordIdx--;
		}
		pVertData->texCoord = texCoordBuffer.at(texCoordIdx);
	}


	size_t vertTokenIdx = 2;
	if(vertNormalBuffer.size()>0){
		if(texCoordBuffer.size()==0) vertTokenIdx = 1;
		token = vertTokenList.at(vertTokenIdx);
		int64_t normIdx = 0;
		std::from_chars(token.data(), token.data()+token.size(), normIdx);
		if(normIdx<0){
			normIdx += vertNormalBuffer.size();
		}
		else{
			normIdx--;
		}
		pVertData->normal = vertNormalBuffer.at(normIdx);
	}
}

void FileTools::Obj::CloseFile(){
	::UnmapViewOfFile(pView_);
	::CloseHandle(hMap_);
	::CloseHandle(hFile_);

}
void FileTools::Obj::PushBackData(
	const std::vector<SubMesh> &alphaTestedMeshData,
	const std::vector<VertexPosTexNorm> &alphaTestedVertexData,
	const std::vector<uint32_t> &alphaTestedIndexData,
	std::vector<VertexPosTexNorm> *pIndexedVertexData,
	std::vector<uint32_t> *pIndexData,
	std::vector<SubMesh> *pMeshOffsetData) const{
	const uint32_t vertexOffset = pIndexedVertexData->size();
	const uint32_t indexOffset = pIndexData->size();
	pIndexedVertexData->resize(vertexOffset+alphaTestedVertexData.size());
	pIndexData->resize(indexOffset+alphaTestedIndexData.size());
	for(uint32_t i = 0; i<alphaTestedVertexData.size(); ++i){
		(*pIndexedVertexData)[vertexOffset+i] = alphaTestedVertexData[i];
	}
	for(uint32_t i = 0; i<alphaTestedIndexData.size(); ++i){
		(*pIndexData)[indexOffset+i] = alphaTestedIndexData[i] + vertexOffset;
	}
	for(uint32_t i = 0; i<alphaTestedMeshData.size(); ++i){
		SubMesh temp = alphaTestedMeshData[i];
		pMeshOffsetData->push_back({temp.objName, temp.groupName, temp.material, temp.indexOffset+indexOffset, temp.numIndices, temp.vertexOffset+ vertexOffset, temp.numVertices, true});
	}

}
