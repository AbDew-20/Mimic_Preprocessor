#include <Utils/FileTools.h>
#include <Core/PCH.h>
#include <fstream>
#include <charconv>
#include <thirdParty/meshoptimizer/meshoptimizer.h>
#include <thirdParty/DirectXTex/DirectXTex.h>




namespace{
	enum class ObjHeaderCode{
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

	enum class MtlHeaderCode{
		materialId,
		diffuse,
		specular,
		normal,
		dissolve,
		metalness,
		roughness,
		emissive,
		undef
	};

	ObjHeaderCode HashObjHeader(const std::string_view header){
		if(header=="mtllib") return ObjHeaderCode::mtlFile;
		if(header=="v") return ObjHeaderCode::vertex;
		if(header=="vt") return ObjHeaderCode::texture;
		if(header=="vn") return ObjHeaderCode::normal;
		if(header=="g") return ObjHeaderCode::group;
		if(header=="o") return ObjHeaderCode::object;
		if(header=="usemtl") return ObjHeaderCode::material;
		if(header=="f") return ObjHeaderCode::face;
		return ObjHeaderCode::undef;
	}

	MtlHeaderCode HashMtlHeader(const std::string_view header){
		if(header=="newmtl") return MtlHeaderCode::materialId;
		if(header=="map_Kd") return MtlHeaderCode::diffuse;
		if(header=="map_d") return MtlHeaderCode::dissolve;
		if(header=="map_Ks") return MtlHeaderCode::specular;
		if(header=="map_Bump") return MtlHeaderCode::normal;
		if(header=="map_Pm") return MtlHeaderCode::metalness;
		if(header=="map_Pr") return MtlHeaderCode::roughness;
		if(header=="map_Ke") return MtlHeaderCode::emissive;
		return MtlHeaderCode::undef;
	}



	bool ParseString(std::string_view string, const char delim, std::vector<std::string_view> *pTokenList){
		size_t runningOffset = 0;
		while(string.size()>runningOffset){
			size_t offset = string.find_first_of(delim, runningOffset);
			if(offset==std::string::npos){
				offset = string.size();
			}
			if(offset!=runningOffset) pTokenList->emplace_back(string.substr(runningOffset, offset-runningOffset));
			runningOffset = offset+1;
		}
		return true;

	}

	void ParseLines(size_t startOffset,const char *pBuffer, size_t bufferSize, std::vector<std::string_view> *pLines){
		const char *end = pBuffer+bufferSize;
		const char *lastLine = end;
		const char *startView = pBuffer;
		const char *start = startView+startOffset;
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

}

struct FileTools::Mtl::MaterialTextures{
	std::string diffuse;
	std::string specular;
	std::string normal;
	std::string dissolve;
	std::string metalness;
	std::string roughness;
	std::string emissive;
};

void FileTools::LoadFileToBuffer(const std::string &filePath, std::vector<char> *pBuffer){
	std::ifstream readFile(filePath.data(), std::ios::binary|std::ios::ate);
	assert(readFile.is_open()&&"Error opening file");
	size_t fileSize = readFile.tellg();
	pBuffer->assign(fileSize, '\0');
	readFile.seekg(0);
	readFile.read(pBuffer->data(), fileSize);
	readFile.close();
}

void FileTools::WriteBufferToFile(const std::string &filePath, const std::vector<char> &buffer){
	std::ofstream file(filePath.data(), std::ofstream::trunc);
	assert(file.is_open()&&"Erroe creating and opening file");
	file<<buffer.data();
	file.close();
}



FileTools::Obj::Obj(const std::string &filePath) :
	filePath_(filePath){
	SYSTEM_INFO sysInfo = {};
	::GetSystemInfo(&sysInfo);
	allocGranularity_ = sysInfo.dwAllocationGranularity;
	pageSize_ = allocGranularity_*512;
	std::vector<std::string_view> tokenList;
	ParseString(filePath_, '\\', &tokenList);
	for(int i = 0; i<tokenList.size()-1; ++i){
		directory_.append(tokenList[i]);
		directory_.append("/");
	}
}


void FileTools::Obj::MapFile(){
	hFile_=::CreateFileA(filePath_.c_str(), GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
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
		std::vector<VertexPosTexNorm> &indexedVertexBuffer,
		std::vector<uint32_t> &indexBuffer,
		std::vector<SubMesh> &meshOffsetData,
		std::vector<MaterialInfo> &matierialInfoData,
		std::unordered_map<std::string, size_t> &materialIdMap){

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
			ParseString(lineSv, ' ', &tokenList);
			header = tokenList.at(0);
			ObjHeaderCode hCode = HashObjHeader(header);
			switch(hCode){
			case ObjHeaderCode::mtlFile:
			{
				std::string materialFile(tokenList.at(1));
				Mtl mtlLoader(materialFile, directory_);
				mtlLoader.ParseMtlFile(matierialInfoData, materialIdMap);
			}
			break;
			case ObjHeaderCode::face:
			{
				size_t listSize = tokenList.size();
				size_t numTriangles = listSize-3;
				token = tokenList.at(1);
				vertTokenList.clear();
				ParseString(token, '/', &vertTokenList);
				VertexPosTexNorm vert0;
				Obj::LoadVertexData(vertTokenList, vertPosBuffer,
					texCoordBuffer, vertNormalBuffer, &vert0);


				for(size_t i = 0; i<numTriangles; ++i){
					interleavedBuffer.push_back(vert0);
					for(size_t j = 0; j<2; ++j){
						token = tokenList.at((i+2+j));
						vertTokenList.clear();
						ParseString(token, '/', &vertTokenList);
						Obj::LoadVertexData(vertTokenList, vertPosBuffer,
							texCoordBuffer, vertNormalBuffer, &vertData);
						interleavedBuffer.push_back(vertData);
					}


				}
			}

			break;
			case ObjHeaderCode::vertex:
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
			case ObjHeaderCode::texture:
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

			case ObjHeaderCode::normal:
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
			case ObjHeaderCode::group:
			{
				if(!interleavedBuffer.empty()){
					std::string meshName;
					meshName.append(objectName);
					meshName.append(groupName);
					meshName.append(mtlName);
					bool alphaTested = matierialInfoData.at(materialIdMap.at(mtlName)).alphaTested;
					meshOffsetData.push_back({objectName,groupName, mtlName, indexBuffer.size(), interleavedBuffer.size(), indexedVertexBuffer.size(), 0, alphaTested});
					Obj::GenerateIndexBuffer(interleavedBuffer, indexedVertexBuffer, indexBuffer);
					meshOffsetData.back().numVertices = indexedVertexBuffer.size()-meshOffsetData.back().vertexOffset;
					interleavedBuffer.clear();
				}
				if(!alphaTestedMeshData.empty()&&objectName==""){
					Obj::PushBackData(alphaTestedMeshData, alphaTestedVertexData, alphaTestedIndexData, indexedVertexBuffer, indexBuffer, meshOffsetData);
					alphaTestedMeshData.clear();
					alphaTestedIndexData.clear();
					alphaTestedVertexData.clear();
				}
				groupName = tokenList.at(1);

			}
			break;
			case ObjHeaderCode::object:
			{
				if(!interleavedBuffer.empty()){
					std::string meshName;
					meshName.append(objectName);
					meshName.append(groupName);
					meshName.append(mtlName);
					bool alphaTested = matierialInfoData.at(materialIdMap.at(mtlName)).alphaTested;
					meshOffsetData.push_back({objectName,groupName, mtlName, indexBuffer.size(), interleavedBuffer.size(), indexedVertexBuffer.size(), 0, alphaTested});
					Obj::GenerateIndexBuffer(interleavedBuffer, indexedVertexBuffer, indexBuffer);
					meshOffsetData.back().numVertices = indexedVertexBuffer.size()-meshOffsetData.back().vertexOffset;
					interleavedBuffer.clear();
				}
				if(!alphaTestedMeshData.empty()){
					Obj::PushBackData(alphaTestedMeshData, alphaTestedVertexData, alphaTestedIndexData, indexedVertexBuffer, indexBuffer, meshOffsetData);
					alphaTestedMeshData.clear();
					alphaTestedIndexData.clear();
					alphaTestedVertexData.clear();
				}
				objectName = tokenList.at(1);
			
			}
			break;
			case ObjHeaderCode::material:
			{
				if(!interleavedBuffer.empty()){
					std::string meshName;
					meshName.append(objectName);
					meshName.append(groupName);
					bool alphaTested = matierialInfoData.at(materialIdMap.at(mtlName)).alphaTested;
					if(alphaTested && meshName!=""){
						alphaTestedMeshData.push_back({objectName, groupName, mtlName, alphaTestedIndexData.size(), interleavedBuffer.size(), alphaTestedVertexData.size(), 0, alphaTested});
						Obj::GenerateIndexBuffer(interleavedBuffer, alphaTestedVertexData, alphaTestedIndexData);
						alphaTestedMeshData.back().numVertices = alphaTestedIndexData.size()-alphaTestedMeshData.back().vertexOffset;
						interleavedBuffer.clear();
					}
					else{
						meshOffsetData.push_back(SubMesh{objectName,groupName, mtlName, indexBuffer.size(), interleavedBuffer.size(), indexedVertexBuffer.size(), 0, alphaTested});
						Obj::GenerateIndexBuffer(interleavedBuffer, indexedVertexBuffer, indexBuffer);
						meshOffsetData.back().numVertices = indexedVertexBuffer.size()-meshOffsetData.back().vertexOffset;
						interleavedBuffer.clear();
					}
				}
				mtlName = tokenList.at(1);

			}
			break;
			case ObjHeaderCode::undef:
				continue;
				break;

			}

		}
	}
	bool alphaTested = matierialInfoData.at(materialIdMap.at(mtlName)).alphaTested;
	meshOffsetData.push_back({objectName,groupName, mtlName, indexBuffer.size(), interleavedBuffer.size(), indexedVertexBuffer.size(), 0, alphaTested});
	Obj::GenerateIndexBuffer(interleavedBuffer, indexedVertexBuffer, indexBuffer);
	meshOffsetData.back().numVertices = indexedVertexBuffer.size()-meshOffsetData.back().vertexOffset;
}


void FileTools::Obj::GenerateIndexBuffer(const std::vector<VertexPosTexNorm> &interleavedBuffer, std::vector<VertexPosTexNorm> &indexedInterleavedBuffer, std::vector<uint32_t> &indexBuffer) const{
	size_t numIndices = interleavedBuffer.size();
	std::vector<uint32_t> remap(numIndices);
	size_t numVertices = meshopt_generateVertexRemap(remap.data(), nullptr, numIndices, interleavedBuffer.data(), numIndices, sizeof(VertexPosTexNorm));
	size_t vertexBufferOffset = indexedInterleavedBuffer.size();
	size_t indexBufferOffset = indexBuffer.size();
	indexedInterleavedBuffer.resize(vertexBufferOffset+ numVertices);
	indexBuffer.resize(indexBufferOffset+ numIndices);
	meshopt_remapVertexBuffer(indexedInterleavedBuffer.data() +vertexBufferOffset, interleavedBuffer.data(), numIndices, sizeof(VertexPosTexNorm), remap.data());
	meshopt_remapIndexBuffer(indexBuffer.data()+indexBufferOffset, nullptr, numIndices, remap.data());
	for(auto iter = indexBuffer.begin()+indexBufferOffset; iter!=indexBuffer.end(); ++iter){
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
	std::vector<VertexPosTexNorm> &indexedVertexData,
	std::vector<uint32_t> &indexData,
	std::vector<SubMesh> &meshOffsetData) const{
	const uint32_t vertexOffset = indexedVertexData.size();
	const uint32_t indexOffset = indexData.size();
	indexedVertexData.resize(vertexOffset+alphaTestedVertexData.size());
	indexData.resize(indexOffset+alphaTestedIndexData.size());
	for(uint32_t i = 0; i<alphaTestedVertexData.size(); ++i){
		indexedVertexData[vertexOffset+i] = alphaTestedVertexData[i];
	}
	for(uint32_t i = 0; i<alphaTestedIndexData.size(); ++i){
		indexData[indexOffset+i] = alphaTestedIndexData[i] + vertexOffset;
	}
	for(uint32_t i = 0; i<alphaTestedMeshData.size(); ++i){
		SubMesh temp = alphaTestedMeshData[i];
		meshOffsetData.push_back({temp.objName, temp.groupName, temp.material, temp.indexOffset+indexOffset, temp.numIndices, temp.vertexOffset+ vertexOffset, temp.numVertices, true});
	}

}


FileTools::Mtl::Mtl(const std::string mtlFile, const std::string directory):
fileName_(mtlFile),
currentDir_(directory){
}
void FileTools::Mtl::ParseMtlFile(std::vector<MaterialInfo> &matierialInfoData, std::unordered_map<std::string, size_t> &materialIdMap){
	matierialInfoData.reserve(50);
	matierialInfoData.clear();
	materialIdMap.clear();

	ThrowIfFailed(::CoInitializeEx(nullptr, COINIT_MULTITHREADED));
	std::vector<char> fileBuffer;
	std::string filePath;
	filePath.append(currentDir_);
	filePath.append(fileName_);
	FileTools::LoadFileToBuffer(filePath, &fileBuffer);
	std::vector<std::string_view> lines;
	ParseLines(0, fileBuffer.data(), fileBuffer.size(), &lines);
	
	std::vector<std::string_view> tokenList;
	tokenList.reserve(5);
	std::string_view header;

	MaterialTextures texturePaths;
	std::string materialId;
	for(auto lineSv:lines){
		tokenList.clear();
		if(!(lineSv.size()>1)){
			continue;
		}
		ParseString(lineSv, ' ', &tokenList);
		header = tokenList.at(0);
		MtlHeaderCode headerCode = HashMtlHeader(header);
		
		switch(headerCode){
		case(MtlHeaderCode::diffuse):
		{
			texturePaths.diffuse = tokenList.at(1);
		}
		break;
		case(MtlHeaderCode::materialId):
		{
			MaterialInfo materialInfo = {};
			if(materialId!=tokenList.at(1)){
				Mtl::GenerateMaterialData(texturePaths, &materialInfo);
				matierialInfoData.push_back(materialInfo);
				materialIdMap.insert({materialId,matierialInfoData.size()-1}) ;
				materialId = tokenList.at(1);
				texturePaths = {};
			}
		}
		break;
		case(MtlHeaderCode::specular):
		{
			texturePaths.specular = tokenList.at(1);
		}
		break;
		case(MtlHeaderCode::dissolve):
		{
			texturePaths.dissolve = tokenList.at(1);
		}
		break;
		case(MtlHeaderCode::emissive):
		{
			texturePaths.emissive = tokenList.at(1);
		}
		break;
		case(MtlHeaderCode::normal):
		{
			texturePaths.normal = tokenList.at(3);
		}
		break;

		default:
		{
		}
		break;
		}
	}
	MaterialInfo materialInfo = {};
	Mtl::GenerateMaterialData(texturePaths, &materialInfo);
	matierialInfoData.push_back(materialInfo);
	materialIdMap.insert({materialId,matierialInfoData.size()-1}) ;
	texturePaths = {};

	::CoUninitialize();

}

void FileTools::Mtl::GenerateMaterialData(const MaterialTextures &texturePaths, MaterialInfo *pMaterialInfo){
	if(texturePaths.dissolve==""){
		pMaterialInfo->alphaTested = false;
		return;
	}
	std::vector<std::string_view> tokenList;
	ParseString(texturePaths.dissolve, '.', &tokenList);
	if(tokenList.back()=="dds"){
		DirectX::TexMetadata metaData;
		std::string filePath;
		filePath.append(currentDir_);
		filePath.append(texturePaths.dissolve);
		std::wstring filePathW;
		StringToWString(filePath, filePathW);
		ThrowIfFailed(DirectX::GetMetadataFromDDSFileEx(filePathW.c_str(), DirectX::DDS_FLAGS_NONE, metaData, nullptr));
		pMaterialInfo->alphaTested = (DirectX::HasAlpha(metaData.format)&& (metaData.format!=(DXGI_FORMAT_BC1_UNORM|DXGI_FORMAT_BC1_TYPELESS)));
	}
}

