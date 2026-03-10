#pragma once
#include <Core/PCH.h>

struct MappedInput;
struct LoadingUpdateParams{
	int clientWidth;
	int clientHeight;
	float percent;
	const std::string &label;
};
class LoadingContext{
public:
	LoadingContext(const std::string &label);
	void Update(const LoadingUpdateParams &updateParams, double deltaTime);
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4* pCommandList,double deltaTime);
protected:

private:
	std::string label_;
		
};
