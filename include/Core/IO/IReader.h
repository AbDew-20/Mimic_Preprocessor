#pragma once

class IReader{
public:
	virtual void Read(void *pBuffer, size_t size) = 0;
	virtual void Read(void *pBuffer, size_t size, size_t offset) = 0;
	virtual ~IReader() = default;

};