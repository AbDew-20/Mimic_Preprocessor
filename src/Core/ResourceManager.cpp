#include <Core/PCH.h>
#include <Core/ResourceManager.h>
#include <Core/Application.h>


ResourceManager::ResourceManager(Application* pApp):
pApp_(pApp){

}



void ResourceManager::UpdateBufferResource(ID3D12GraphicsCommandList4 *pCommandList,
	ID3D12Resource **ppDestinationResource,
	ID3D12Resource **ppStagingResource,
	size_t numElements, size_t elementSize,
	const void *buffer, D3D12_RESOURCE_FLAGS flags){
	ID3D12Device2 *pDevice = pApp_->GetDevice();
	size_t bufferSize = elementSize*numElements;

	D3D12_HEAP_PROPERTIES heapProp=::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);
	D3D12_RESOURCE_DESC resourceDesc=::CD3DX12_RESOURCE_DESC::Buffer(bufferSize, flags);
	ThrowIfFailed(pDevice->CreateCommittedResource(
		&heapProp,
		D3D12_HEAP_FLAG_NONE,
		&resourceDesc,
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_ID3D12Resource,
		reinterpret_cast<void **>(ppDestinationResource)));

	if(buffer){
		D3D12_HEAP_PROPERTIES heapProp=::CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD);
		D3D12_RESOURCE_DESC resourceDesc=::CD3DX12_RESOURCE_DESC::Buffer(bufferSize);
		ThrowIfFailed(pDevice->CreateCommittedResource(
			&heapProp,
			D3D12_HEAP_FLAG_NONE,
			&resourceDesc,
			D3D12_RESOURCE_STATE_GENERIC_READ,
			nullptr,
			IID_ID3D12Resource,
			reinterpret_cast<void **>(ppStagingResource)
		));
		
		D3D12_SUBRESOURCE_DATA subresourceData = {};
		subresourceData.pData = buffer;
		subresourceData.RowPitch = bufferSize;
		subresourceData.SlicePitch = subresourceData.RowPitch;


		::UpdateSubresources(pCommandList, *ppDestinationResource, *ppStagingResource, 0, 0, 1, &subresourceData);

	}
}
uint64_t ResourceManager::QueueVertexBuffer(const void *pBuffer,
	const size_t numElements,
	const size_t elementSize,
	BufferResource *pVertexBufferResource){
	
	CommandQueue *pCommandQueue = pApp_->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	ID3D12GraphicsCommandList4 *pCommandList = pCommandQueue->GetCommandList(); 

	ID3D12Resource *pStagingBuffer = nullptr;
	UpdateBufferResource(pCommandList, &pVertexBufferResource->pBuffer, &pStagingBuffer,
		numElements, elementSize, pBuffer, D3D12_RESOURCE_FLAG_NONE);

	pVertexBufferResource->vertexView.BufferLocation = pVertexBufferResource->pBuffer->GetGPUVirtualAddress();
	pVertexBufferResource->vertexView.SizeInBytes = (UINT)numElements*elementSize;
	pVertexBufferResource->vertexView.StrideInBytes = elementSize;
	uint64_t fence= pCommandQueue->ExecuteCommandList(pCommandList);
	uploadTickets_.push_back({pStagingBuffer, fence});
	return fence;
}
uint64_t ResourceManager::QueueIndexBuffer(const void *pBuffer,
	const size_t numElements,
	BufferResource *pIndexBufferResource){

	CommandQueue *pCommandQueue = pApp_->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	ID3D12GraphicsCommandList4 *pCommandList = pCommandQueue->GetCommandList(); 
	
	ID3D12Resource *pStagingBuffer = nullptr;
	UpdateBufferResource(pCommandList, &pIndexBufferResource->pBuffer, &pStagingBuffer,
		numElements, sizeof(uint32_t), pBuffer, D3D12_RESOURCE_FLAG_NONE);
	pIndexBufferResource->indexView.BufferLocation = pIndexBufferResource->pBuffer->GetGPUVirtualAddress();
	pIndexBufferResource->indexView.SizeInBytes = sizeof(uint32_t)*(UINT)numElements;
	pIndexBufferResource->indexView.Format = DXGI_FORMAT_R32_UINT;
	uint64_t fence= pCommandQueue->ExecuteCommandList(pCommandList);
	uploadTickets_.push_back({pStagingBuffer, fence});
	return fence;

}

void ResourceManager::UploadVertexBuffer(const void *pBuffer,
	const size_t numElements,
	const size_t elementSize,
	BufferResource *pVertexBufferResource
){
	uint64_t fence = QueueVertexBuffer(pBuffer, numElements, elementSize, pVertexBufferResource);
	CommandQueue *pCommandQueue = pApp_->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	pCommandQueue->WaitForFenceValue(fence);
	SafeRelease(uploadTickets_.back().pStagingBuffer);
	uploadTickets_.pop_back();
}

void ResourceManager::UploadIndexBuffer(const void *pBuffer,
	const size_t numElements,
	BufferResource *pIndexBufferResource
){
	uint64_t fence = QueueIndexBuffer(pBuffer, numElements, pIndexBufferResource);
	CommandQueue *pCommandQueue = pApp_->GetCommandQueue(D3D12_COMMAND_LIST_TYPE_COPY);
	pCommandQueue->WaitForFenceValue(fence);
	SafeRelease(uploadTickets_.back().pStagingBuffer);
	uploadTickets_.pop_back();
}
