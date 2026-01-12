#pragma once
#define WINDOWS_LEAN_AND_MEAN
#include <windows.h>
#include <d3d12.h>
#include <vector>

class Application;
class DescriptorHeapAllocator{
public:
	DescriptorHeapAllocator(UINT numDescriptorsPerHeap, D3D12_DESCRIPTOR_HEAP_TYPE heapType,D3D12_DESCRIPTOR_HEAP_FLAGS flags, Application* pApp);
	void Alloc(D3D12_CPU_DESCRIPTOR_HANDLE *pCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE* pGpuHandle);
	void Free(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle);
	ID3D12DescriptorHeap *GetHeapPointer(uint32_t heap);
	ID3D12DescriptorHeap* const *GetHeapPointerLocation(uint32_t heap);
	void Destroy();
protected:
	
private:
	struct HeapInfo{
		ID3D12DescriptorHeap *pHeap;
		D3D12_CPU_DESCRIPTOR_HANDLE cpuStartHandle;
		D3D12_GPU_DESCRIPTOR_HANDLE gpuStartHandle;
	};
	void AddHeap();
	uint32_t GetIndex(D3D12_CPU_DESCRIPTOR_HANDLE);

	std::vector<HeapInfo> heaps_;
	std::vector<size_t> freeList_;
	Application *pApp_;
	size_t top_;
	UINT numDescriptorsPerHeap_;
	UINT descriptorSize_;
	D3D12_DESCRIPTOR_HEAP_TYPE heapType_;
	D3D12_DESCRIPTOR_HEAP_FLAGS flags_;

};
