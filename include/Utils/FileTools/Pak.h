#pragma once
#include <vector>
#include <string>
#include <cstddef>
#include <Core/IO/IWriter.h>
#include <Core/IO/IReader.h>
#include "Core/PCH.h"


namespace FileTools{
	class PakItemReader;
	class PakItemWriter;

	struct PakItemInfo{
		char itemName[56];
		uint32_t offset;
		uint32_t itemSize;
	};
	struct ItemView{
		const std::byte *pItemView;
		DWORD itemSize;
	};
	class Pak{
		struct FileMapping{
			HANDLE hFile;
			HANDLE hMap;
			uint64_t fileSize;
		};
		struct Header{
			uint32_t fileTableOffset;
			uint32_t fileTableSize;
		};
	public:
		Pak(const std::string pakFile, const std::string directory);
		std::vector<PakItemInfo> &ReturnPakInfo(){ return itemInfoData_; }
		void GetItem(uint32_t itemIndex, std::vector<char> &buffer);
		ItemView OpenItemView(uint32_t itemIndex);
		void CloseItemView();
		[[nodiscard]] PakItemReader OpenItemReadStream(uint32_t itemIndex);
		void ReadItemStream(void *pBuffer, size_t size);
		void CloseItemReadStream();
		void AddItem(const std::string &itemName, const std::vector<char> &buffer);
		void OpenPak();
		void ClosePak();
		[[nodiscard]] PakItemWriter OpenItemWriteStream(const std::string &itemName);
		void WriteItemStream(const void *pBuffer, size_t size);
		void CloseItemWriteStream();
		static constexpr uint32_t magicSize = 4U;
		static constexpr uint32_t headerSize = sizeof(Header);

	protected:
		
	private:
		struct WriteStream{
			std::string itemName;
			uint32_t offset;
			uint32_t bytesWritten;
		};
		struct ReadStream{
			ItemView view;
			uint32_t itemIndex;
			uint32_t offset;
		};
		void ParsePak();
		std::string fileName_;
		std::string directory_;
		FileMapping fileMap_;
		LPVOID pActiveView_;
		Header header_;
		WriteStream writeStream_;
		ReadStream readStream_;
		std::vector<PakItemInfo> itemInfoData_;
	};

	class PakItemWriter : public IWriter{
	public:
		PakItemWriter(Pak &pak);
		void Write(const void *pBuffer, size_t size) override;
	private:
		Pak &pak_;
	};

	class PakItemReader : public IReader{
	public:
		PakItemReader(Pak &pak);
		void Read(void *pBuffer, size_t size) override;
		void Read(void *pBuffer, size_t size, size_t offset) override;
	private:
		Pak &pak_;
	};

}