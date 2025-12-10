#include <Core/DescriptorHeapAllocator.h>
#include <Core/Application.h>



DescriptorHeapAllocator::DescriptorHeapAllocator(UINT numDescriptorsPerHeap, D3D12_DESCRIPTOR_HEAP_TYPE heapType, D3D12_DESCRIPTOR_HEAP_FLAGS flags, Application *pApp ) :
	pApp_(pApp),
	numDescriptorsPerHeap_(numDescriptorsPerHeap),
	heapType_(heapType),
	top_(0),
	flags_(flags)
{
	pApp->GetDescriptorHandleIncrementSize(heapType);
	freeList_.resize(numDescriptorsPerHeap_);
	std::iota(freeList_.begin(), freeList_.end(), 0);
	ID3D12DescriptorHeap *pHeap = pApp_->CreateDescriptorHeap(numDescriptorsPerHeap_, heapType_, flags_);
	heaps_.push_back({pHeap,pHeap->GetCPUDescriptorHandleForHeapStart(), pHeap->GetGPUDescriptorHandleForHeapStart()});
}

void DescriptorHeapAllocator::AddHeap(){
	ID3D12DescriptorHeap *pHeap = pApp_->CreateDescriptorHeap(numDescriptorsPerHeap_, heapType_, flags_);
	heaps_.push_back({pHeap,pHeap->GetCPUDescriptorHandleForHeapStart(), pHeap->GetGPUDescriptorHandleForHeapStart()});
	uint32_t end = freeList_.size();
	freeList_.resize(end+numDescriptorsPerHeap_);
	std::iota(freeList_.begin()+end, freeList_.end(), end);
}

ID3D12DescriptorHeap *DescriptorHeapAllocator::GetHeapPointer(uint32_t heap){
	if(heap>=heaps_.size()){
		return nullptr;
	}
	return heaps_[heap].pHeap;
}
void DescriptorHeapAllocator::Alloc(D3D12_CPU_DESCRIPTOR_HANDLE *pCpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE *pGpuHandle){
	if(top_==freeList_.size()){
		AddHeap();
	}
	uint32_t idx = freeList_.at(top_);
	top_++;
	uint32_t heap = idx/numDescriptorsPerHeap_;
	uint32_t index = idx%numDescriptorsPerHeap_;
	pCpuHandle->ptr = heaps_[heap].cpuStartHandle.ptr+(index*descriptorSize_);
	pGpuHandle->ptr = heaps_[heap].gpuStartHandle.ptr+(index*descriptorSize_);
}

void DescriptorHeapAllocator::Free(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle){
	uint32_t idx = GetIndex(cpuHandle);
	if(std::find(freeList_.begin()+top_, freeList_.end(), idx)!=freeList_.end()){
		return;
	}
	top_--;
	freeList_.at(top_) = idx;
}
uint32_t DescriptorHeapAllocator::GetIndex(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle){
	for(int i = 0; i<heaps_.size(); i++){
		int idx = cpuHandle.ptr-heaps_[i].cpuStartHandle.ptr;
		if(idx>=0&&idx<numDescriptorsPerHeap_*descriptorSize_){
			return (uint32_t)(idx+(i*numDescriptorsPerHeap_));
		}
	}
	return 0;
}

void DescriptorHeapAllocator::Destroy(){
	for(int i = 0; i<heaps_.size(); ++i){
		SafeRelease(heaps_[i].pHeap);
	}
}
ID3D12DescriptorHeap *const *DescriptorHeapAllocator::GetHeapPointerLocation(uint32_t heap){
	if(heap>=heaps_.size()){
		return nullptr;
	}
	return &heaps_[heap].pHeap;
}