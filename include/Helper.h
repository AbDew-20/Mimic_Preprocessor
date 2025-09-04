#pragma once

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <exception>

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


