#include <Core/PCH.h>

#include <Core/CommandQueue.h>


CommandQueue::CommandQueue(ID3D12Device2 *pDevice, D3D12_COMMAND_LIST_TYPE type) :
	pDevice_(pDevice),
	commandListType_(type),
	fenceValue_(0),
	fenceEvent_(nullptr),
	pCommandQueue_(nullptr),
	pFence_(nullptr){

}
void CommandQueue::Init(){
	D3D12_COMMAND_QUEUE_DESC desc = {};
	desc.Type = commandListType_;
	desc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
	desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	desc.NodeMask = 0;

	ThrowIfFailed(pDevice_->CreateCommandQueue(&desc, IID_PPV_ARGS(&pCommandQueue_)));
	ThrowIfFailed(pDevice_->CreateFence(fenceValue_, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&pFence_)));

	fenceEvent_ = ::CreateEvent(NULL, FALSE, FALSE, NULL);
	assert(fenceEvent_&&"Failed to create fence event handle");
}

void CommandQueue::Destroy(){
	while(!commandListQueue_.empty()){
		ID3D12GraphicsCommandList2* pCommandList= commandListQueue_.front();
		SafeRelease(pCommandList);
		commandListQueue_.pop();
	}
	while(!commandAllocatorQueue_.empty()){
		CommandAllocatorData data = commandAllocatorQueue_.front();
		if(data.ownsRef){
			data.pCommandAllocator->Release();
			data.ownsRef = false;
		} 
		SafeRelease(data.pCommandAllocator);
		commandAllocatorQueue_.pop();
	}
	SafeRelease(pFence_);
	SafeRelease(pCommandQueue_);

}

uint64_t CommandQueue::Signal(){
	uint64_t fenceValue = ++fenceValue_;
	pCommandQueue_->Signal(pFence_, fenceValue);
	return fenceValue;
}


bool CommandQueue::IsFenceComplete(uint64_t fenceValue){
	return pFence_->GetCompletedValue()>=fenceValue;
}

void CommandQueue::WaitForFenceValue(uint64_t fenceValue){
	if(!IsFenceComplete(fenceValue)){
		pFence_->SetEventOnCompletion(fenceValue, fenceEvent_);
		::WaitForSingleObject(fenceEvent_, DWORD_MAX);
	}
}


void CommandQueue::Flush(){
	WaitForFenceValue(Signal());
}

ID3D12CommandAllocator *CommandQueue::CreateComamandAllocator(){
	ID3D12CommandAllocator *pCommandAllocator;
	ThrowIfFailed(pDevice_->CreateCommandAllocator(commandListType_, IID_PPV_ARGS(&pCommandAllocator)));
	return pCommandAllocator;
}

ID3D12GraphicsCommandList4 *CommandQueue::CreateCommandList(ID3D12CommandAllocator *pAllocator){
	ID3D12GraphicsCommandList4 *pCommandList;
	ThrowIfFailed(pDevice_->CreateCommandList(0, commandListType_, pAllocator, nullptr, IID_PPV_ARGS(&pCommandList)));
	return pCommandList;
}

ID3D12GraphicsCommandList4 *CommandQueue::GetCommandList(){
	ID3D12CommandAllocator *pCommandAllocator;
	ID3D12GraphicsCommandList4 *pCommandList;
	if(!commandAllocatorQueue_.empty()&&IsFenceComplete(commandAllocatorQueue_.front().fenceValue)){
		CommandAllocatorData commandAllocatorData = commandAllocatorQueue_.front();
		pCommandAllocator = commandAllocatorData.pCommandAllocator;
		commandAllocatorQueue_.pop();
		ThrowIfFailed(pCommandAllocator->Reset());
		if(commandAllocatorData.ownsRef){
			pCommandAllocator->Release();
			commandAllocatorData.ownsRef = false;
		}
	}
	else{
		pCommandAllocator = CreateComamandAllocator();
	}
	if(!commandListQueue_.empty()){
		pCommandList = commandListQueue_.front();
		commandListQueue_.pop();
		ThrowIfFailed(pCommandList->Reset(pCommandAllocator, nullptr));
	}
	else{
		pCommandList = CreateCommandList(pCommandAllocator);
	}
	ThrowIfFailed(pCommandList->SetPrivateDataInterface(IID_ID3D12CommandAllocator, pCommandAllocator));
	return pCommandList;

}


uint64_t CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList4 *pCommandList){
	pCommandList->Close();
	ID3D12CommandAllocator *pCommandAllocator;
	UINT dataSize = sizeof(pCommandAllocator);
	ThrowIfFailed(pCommandList->GetPrivateData(IID_ID3D12CommandAllocator, &dataSize, &pCommandAllocator));

	ID3D12CommandList *const ppCommandLists[] = {pCommandList};
	pCommandQueue_->ExecuteCommandLists(1, ppCommandLists);
	uint64_t fenceValue = Signal();

	commandAllocatorQueue_.push(CommandAllocatorData{fenceValue,pCommandAllocator,true});
	commandListQueue_.push(pCommandList);
	return fenceValue;

}

ID3D12CommandQueue *CommandQueue::GetCommandQueue() const{
	return pCommandQueue_;
}