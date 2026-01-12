#pragma once
#include <Core/PCH.h>
#include <string>

struct MappedInput;
struct SplashStateParams{
	int clientWidth;
	int clientHeight;
	bool &fileSelected;
	std::string &fileName;
};
class SplashContext{
public:
	SplashContext();
	void Update(SplashStateParams &stateParams,double deltaTime);
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4* pCommandList,double deltaTime);
	void HandleInput(MappedInput &mappedInput);
protected:

private:
		
};