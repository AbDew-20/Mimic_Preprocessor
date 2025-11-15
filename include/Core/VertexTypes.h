#pragma once
#include <Core/PCH.h>


struct VertexPosTexNorm{
	DirectX::XMFLOAT3 vert;
	DirectX::XMFLOAT2 texCoord;
	DirectX::XMFLOAT3 normal;

	static constexpr D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}};

	VertexPosTexNorm() = default;

	VertexPosTexNorm(const DirectX::XMFLOAT3 &v,
		const DirectX::XMFLOAT2 &t,
		const DirectX::XMFLOAT3 &n)
		: vert(v), texCoord(t), normal(n){}
};

struct VertexPos{
	DirectX::XMFLOAT3 vert;

	static constexpr D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};
	VertexPos() = default;
	VertexPos(const DirectX::XMFLOAT3 &v) :
		vert(v)
	{}
};
