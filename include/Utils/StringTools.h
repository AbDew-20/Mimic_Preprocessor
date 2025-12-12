#pragma once
#include <vector>
#include <string_view>

namespace StringTools{

	void ParseString(std::string_view string, const char delim, std::vector<std::string_view> *pTokenList){
		size_t runningOffset = 0;
		while(string.size()>runningOffset){
			size_t offset = string.find_first_of(delim, runningOffset);
			if(offset==std::string::npos){
				offset = string.size();
			}
			if(offset!=runningOffset) pTokenList->emplace_back(string.substr(runningOffset, offset-runningOffset));
			runningOffset = offset+1;
		}

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