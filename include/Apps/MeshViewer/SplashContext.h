#pragma once
#include <Core/PCH.h>
#include <string>

struct MappedInput;
struct SplashStateParams{
	std::string fileName;
	bool fileSelected;
};
struct SplashUpdateParams{
	int clientWidth;
	int clientHeight;
};
class SplashContext{
public:
	SplashContext();
	void Update(const SplashUpdateParams &updateParams,double deltaTime, SplashStateParams &stateParams);
	void Render(D3D12_CPU_DESCRIPTOR_HANDLE rtv, D3D12_CPU_DESCRIPTOR_HANDLE dsv, ID3D12GraphicsCommandList4* pCommandList,double deltaTime);
	void HandleInput(MappedInput &mappedInput);
protected:

private:
	struct SplashStateVariables{
		std::string fileName;
	};
	SplashStateVariables stateVariables_;
};