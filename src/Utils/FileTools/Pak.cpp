#include "Utils/FileTools/Pak.h"




FileTools::Pak::Pak(const std::string pakFile, const std::string directory)
	:fileName_(pakFile),
	directory_(directory),
	header_({0,0}){
}

void FileTools::Pak::OpenPak(){
	std::string filePath(directory_+fileName_);
	DWORD fileAttrib = ::GetFileAttributesA(filePath.c_str());
	if(fileAttrib != INVALID_FILE_ATTRIBUTES){
		HANDLE hFile=::CreateFileA(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);
		assert(!(hFile==INVALID_HANDLE_VALUE)&&"Failed to create file handle");
		fileMap_.hFile = hFile;
		LARGE_INTEGER fileSize = {};
		GetFileSizeEx(hFile, &fileSize);
		fileMap_.fileSize = fileSize.QuadPart;
		HANDLE hMap = ::CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
		assert(!(hMap==INVALID_HANDLE_VALUE)&&"Failed to map file");
		fileMap_.hMap = hMap;
		ParsePak();
	}
	else{
		HANDLE hFile=::CreateFileA(filePath.c_str(), GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, CREATE_NEW, FILE_ATTRIBUTE_NORMAL, NULL);
		assert(!(hFile==INVALID_HANDLE_VALUE)&&"Failed to create file handle");
		fileMap_.hFile = hFile;
		const char magic[4] = {'P', 'A', 'C', 'K'};
		const Header header = {12, 0};
		DWORD bytesWritten = 0;
		ThrowIfFailed(::WriteFile(hFile, &magic, (DWORD)4, &bytesWritten, NULL));
		ThrowIfFailed(::WriteFile(hFile, &header, (DWORD)sizeof(Header), &bytesWritten, NULL));
		fileMap_.fileSize = 12;
		header_.fileTableOffset = 12;
		header_.fileTableSize = 0;
		HANDLE hMap = ::CreateFileMappingA(hFile, NULL, PAGE_READWRITE, 0, 0, NULL);
		assert(!(hMap==INVALID_HANDLE_VALUE)&&"Failed to map file");
		fileMap_.hMap = hMap;
	}

}

void FileTools::Pak::ClosePak(){
	OVERLAPPED overlap = {0};
	overlap.Offset = (DWORD) header_.fileTableOffset;
	ThrowIfFailed(::WriteFileEx(fileMap_.hFile, itemInfoData_.data(), (DWORD) header_.fileTableSize, &overlap, NULL));

	overlap.Offset = 4;
	ThrowIfFailed(::WriteFileEx(fileMap_.hFile, &header_, (DWORD) 8, &overlap, NULL));
	LARGE_INTEGER offset = {header_.fileTableOffset + header_.fileTableSize, 0L};
	::SetFilePointerEx(fileMap_.hFile, offset, NULL, 0U);
	::SetEndOfFile(fileMap_.hFile);
	::CloseHandle(fileMap_.hMap);
	::CloseHandle(fileMap_.hFile);
}

void FileTools::Pak::AddItem(const std::string &itemName, const std::vector<char> &buffer){
	PakItemInfo itemInfo = {};
	memcpy(&itemInfo.itemName, itemName.substr(0, 56).c_str(), 56);
	itemInfo.offset = header_.fileTableOffset;
	itemInfo.itemSize = buffer.size();
	OVERLAPPED overlap = {0};
	overlap.Offset = itemInfo.offset;
	ThrowIfFailed(::WriteFileEx(fileMap_.hFile, buffer.data(), itemInfo.itemSize, &overlap, NULL));
	header_.fileTableSize += 64UL;
	header_.fileTableOffset += itemInfo.itemSize;
	itemInfoData_.push_back(itemInfo);
}

void FileTools::Pak::GetItem(uint32_t itemIndex, std::vector<char> &buffer){
	uint32_t size = itemInfoData_.at(itemIndex).itemSize;
	uint32_t offset = itemInfoData_.at(itemIndex).offset;
	buffer.resize(size);
	OVERLAPPED overlap = {0};
	overlap.Offset = offset;
	ThrowIfFailed(::ReadFileEx(fileMap_.hFile, buffer.data(), size, &overlap, NULL));
}

void FileTools::Pak::PopItem(){
	if(itemInfoData_.size()==0){
		return;
	}
	header_.fileTableOffset -= itemInfoData_.back().itemSize;
	header_.fileTableSize -= 64UL;
	itemInfoData_.pop_back();
}


FileTools::ItemView FileTools::Pak::OpenItemView(uint32_t itemIndex){
	if(pActiveView_){
		::UnmapViewOfFile(pActiveView_);
	}
	SYSTEM_INFO sysInfo = {};
	::GetSystemInfo(&sysInfo);
	DWORD allocGranularity = sysInfo.dwAllocationGranularity;

	uint32_t size = itemInfoData_.at(itemIndex).itemSize;
	uint32_t offset = itemInfoData_.at(itemIndex).offset;

	DWORD pageOffset = (offset/allocGranularity)*allocGranularity;
	DWORD pageSize = offset-pageOffset+size;
	DWORD offsetHi = 0UL;
	pActiveView_ = ::MapViewOfFile(fileMap_.hMap, FILE_MAP_READ, offsetHi, pageOffset, pageSize);
	assert(!(pActiveView_==NULL)&&"Failed to load view from map");
	std::byte *pItemView = static_cast<std::byte *>(pActiveView_)+(offset-pageOffset);
	return {pItemView, (DWORD)size};
}

void FileTools::Pak::CloseItemView(){
	if(pActiveView_){
		::UnmapViewOfFile(pActiveView_);
	}
}
FileTools::PakItemWriter FileTools::Pak::OpenItemWriteStream(const std::string &itemName){
	assert(writeStream_.offset !=0, "Stream already open");
	writeStream_.itemName = itemName;
	writeStream_.offset = header_.fileTableOffset;
	writeStream_.bytesWritten = 0UL;
	return PakItemWriter(*this);
}
void FileTools::Pak::WriteItemStream(const void *pBuffer, size_t size){
	if(writeStream_.offset==0U){
		return;
	}
	OVERLAPPED overlap = {0};
	overlap.Offset = writeStream_.offset;
	ThrowIfFailed(::WriteFileEx(fileMap_.hFile, pBuffer, size, &overlap, NULL));
	writeStream_.bytesWritten += size;
	writeStream_.offset += size;
}
void FileTools::Pak::CloseItemWriteStream(){
	if(writeStream_.offset==0U){
		return;
	}
	PakItemInfo itemInfo = {};
	itemInfo.offset = header_.fileTableOffset;
	memcpy(&itemInfo.itemName, writeStream_.itemName.substr(0, 56).c_str(), 56);
	itemInfo.itemSize = writeStream_.bytesWritten;
	header_.fileTableSize += sizeof(PakItemInfo);
	header_.fileTableOffset += itemInfo.itemSize;
	itemInfoData_.push_back(itemInfo);
	writeStream_.offset = 0U;
	writeStream_.bytesWritten = 0U;
}

FileTools::PakItemReader FileTools::Pak::OpenItemReadStream(uint32_t itemIndex){
	readStream_.offset = 0U;
	readStream_.view = OpenItemView(itemIndex);
	return PakItemReader(*this);
}
void FileTools::Pak::ReadItemStream(void *pBuffer, size_t size){
	bool validRead = (readStream_.view.pItemView !=nullptr);
	validRead &= (readStream_.view.itemSize>= readStream_.offset+size);
	if(!validRead){
		return;
	}
	memcpy(pBuffer, readStream_.view.pItemView+readStream_.offset, size);
	readStream_.offset += size;
}
void FileTools::Pak::CloseItemReadStream(){
	CloseItemView();
	readStream_.view.pItemView = nullptr;
	readStream_.offset = 0U;
}
void FileTools::Pak::ParsePak(){
	OVERLAPPED overlap{0};
	overlap.Offset = 0;
	char magic[4] = {};
	const char pack[4] = {'P', 'A', 'C', 'K'};
	ThrowIfFailed(::ReadFileEx(fileMap_.hFile, magic, magicSize, &overlap, NULL));
	if(memcmp(magic, pack, magicSize)!=0){
		return;
	}
	Header header = {};
	overlap.Offset = magicSize;
	ThrowIfFailed(::ReadFileEx(fileMap_.hFile, &header, headerSize, &overlap, NULL));
	itemInfoData_.resize(header.fileTableSize/(uint32_t)sizeof(PakItemInfo));
	overlap.Offset = header.fileTableOffset;
	header_ = header;
	ThrowIfFailed(::ReadFileEx(fileMap_.hFile, itemInfoData_.data(), header.fileTableSize, &overlap, NULL));

}


FileTools::PakItemWriter::PakItemWriter(Pak &pak):
	pak_(pak){
}

void FileTools::PakItemWriter::Write(const void *pBuffer, size_t size){
	pak_.WriteItemStream(pBuffer, size);
}

FileTools::PakItemReader::PakItemReader(Pak &pak) :
	pak_(pak){
}
void FileTools::PakItemReader::Read(void *pBuffer, size_t size){
	pak_.ReadItemStream(pBuffer, size);
}

void FileTools::PakItemReader::Read(void *pBuffer, size_t size, size_t offset){
	return;
}