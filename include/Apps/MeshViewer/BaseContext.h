#pragma once
#include <Core/PCH.h>

struct MappedInput;
class BaseContext{
public:
	BaseContext();
	void Update(double deltaTime);
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4* pCommandList,double deltaTime);
	void HandleInput(MappedInput &mappedInput);

protected:

private:




};