#pragma once

#include <d3d12.h>
#include <queue>
#include <cstdint>

class CommandQueue{
public:
	CommandQueue(ID3D12Device2 *device, D3D12_COMMAND_LIST_TYPE type);

	ID3D12GraphicsCommandList4 *GetCommandList();

	uint64_t ExecuteCommandList(ID3D12GraphicsCommandList4 *pCommandList);
	uint64_t Signal();
	bool IsFenceComplete(uint64_t fenceValue);
	void WaitForFenceValue(uint64_t fenceValue);
	void Flush();
	void Init();
	void Destroy();

	ID3D12CommandQueue *GetCommandQueue()const;

private:
	ID3D12CommandAllocator *CreateComamandAllocator();
	ID3D12GraphicsCommandList4 *CreateCommandList(ID3D12CommandAllocator *pAllocator);

	struct CommandAllocatorData{
		uint64_t fenceValue;
		ID3D12CommandAllocator *pCommandAllocator;
		bool ownsRef;
	};

	D3D12_COMMAND_LIST_TYPE commandListType_;
	ID3D12Device2 *pDevice_;
	ID3D12CommandQueue *pCommandQueue_;
	ID3D12Fence *pFence_;
	HANDLE fenceEvent_;
	uint64_t fenceValue_;

	std::queue<CommandAllocatorData> commandAllocatorQueue_;
	std::queue<ID3D12GraphicsCommandList4 *> commandListQueue_;
};
