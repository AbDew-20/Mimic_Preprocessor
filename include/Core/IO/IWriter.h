#pragma once

class IWriter{
public:
	virtual void Write(const void *pBuffer, size_t size) = 0;
	virtual ~IWriter() = default;
};