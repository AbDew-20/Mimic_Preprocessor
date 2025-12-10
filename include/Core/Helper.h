#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <exception>
#include <string>

inline void ThrowIfFailed(HRESULT hr){
	if(FAILED(hr)){
		throw std::exception();
	}
}


template<typename T>
auto SafeRelease(T *&ptr)->decltype(ptr->Release(),void()){
	if(ptr){
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

inline void StringToWString(const std::string &string, std::wstring *pWstring){
	const int size = ::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,string.data(),string.size(), nullptr , 0);
	pWstring->resize(size);
	::MultiByteToWideChar(CP_ACP, MB_PRECOMPOSED,string.data(),string.size(), pWstring->data(), size);
}

