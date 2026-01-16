#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <string>
#include <exception>
#include <intrin.h>
inline void ThrowIfFailed(HRESULT hr){
	if(FAILED(hr)){
		__debugbreak();
		std::terminate();
	}
}


template<typename T>
auto SafeRelease(T *&ptr)->decltype(ptr->Release(),void()){
	if(ptr!=nullptr){
		ptr->Release();
		ptr = nullptr;
	}
	
}
template<typename T>
inline void DebugPrint(const char* format, T payload){
	const size_t buffSize = 500;
	char buffer[buffSize];
	::sprintf_s(buffer, buffSize,format,payload);
	::OutputDebugStringA(buffer);

}

inline void StringToWString(const std::string &string, std::wstring &wString){
	const int size = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,string.data(),static_cast<int>(string.size()), nullptr , 0);
	wString.resize(static_cast<size_t>(size));
	::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,string.data(),static_cast<int>(string.size()), wString.data(), size);
}

inline void WStringToString(const std::wstring &wString, std::string &string){
	const int size = ::WideCharToMultiByte(CP_UTF8, 0, wString.data(), static_cast<int>(wString.size()), nullptr, 0, NULL, NULL);
	string.resize(static_cast<size_t>(size));
	::WideCharToMultiByte(CP_UTF8, 0, wString.data(), static_cast<int>(wString.size()), string.data(), size, NULL, NULL);
}

