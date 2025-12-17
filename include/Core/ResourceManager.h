#pragma once
#include <Core/PCH.h>
#include <cstdint>
#include <vector>

class Application;

struct BufferResource{
	ID3D12Resource *pBuffer;
	union{
		D3D12_VERTEX_BUFFER_VIEW vertexView;
		D3D12_INDEX_BUFFER_VIEW indexView;
	};

};
struct BufferUploadTicket{
	ID3D12Resource *pStagingBuffer;
	uint64_t fence;
};
class ResourceManager{
public:
	ResourceManager(Application *pApp);

	uint64_t QueueVertexBuffer(const void* pBuffer,
		const size_t numElements,
		const size_t elementSize,
		BufferResource* pVertexBufferResource);
	uint64_t QueueIndexBuffer(const void* pBuffer,
		const size_t numElements,
		BufferResource* pIndexBufferResource);
	void UploadVertexBuffer(const void *pBuffer,
		const size_t numElements,
		const size_t elementSize,
		BufferResource* pVertexBufferResource
		);
	void UploadIndexBuffer(const void *pBuffer,
		const size_t numElements,
		BufferResource* pIndexBufferResource
		);
protected:

private:
	void UpdateBufferResource(ID3D12GraphicsCommandList4 *pCommandList,
		ID3D12Resource **ppDestinationResource,
		ID3D12Resource **ppStagingResource,
		size_t numElements, size_t elementSize,
		const void *buffer, D3D12_RESOURCE_FLAGS flags);
	
	Application *pApp_;
	std::vector<BufferUploadTicket> uploadTickets_;



};
