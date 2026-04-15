#pragma once
#include <Core/PCH.h>
#include <algorithm>

struct AttributeSemantic{
	enum class Value : uint32_t{
		Undef =0,
		Position,
		Texture,
		Normal,
	};
	static bool isValid(Value val){
		switch(val){
		case Value::Position:
		case Value::Texture:
		case Value::Normal:
			return true;
		default:
			return false;
		}
	}
};


struct AttributeType{
	enum class Value : uint32_t{
		Undef = 0,
		Float32,
	};

	static bool isValid(Value val){
		switch(val){
		case Value::Float32:
			return true;
		default:
			return false;
		}
	}
};


struct VertexAttributeDesc{
	AttributeSemantic::Value semantic = AttributeSemantic::Value::Undef;
	AttributeType::Value type = AttributeType::Value::Undef;
	uint32_t numComponents = 0;
	uint32_t offset = 0;
	uint32_t size = 0;
};
struct VertexLayout{
	uint32_t stride = 0;
	std::vector<VertexAttributeDesc> attributeData;

	bool has(AttributeSemantic::Value semantic, uint32_t numComponents)const{
		return std::any_of(attributeData.begin(), attributeData.end(), [&](const VertexAttributeDesc &desc){return (desc.semantic==semantic)&&(desc.numComponents==numComponents); });
	}
};

struct VertexPosTexNorm{
	DirectX::XMFLOAT3 vert;
	DirectX::XMFLOAT2 texCoord;
	DirectX::XMFLOAT3 normal;

	static constexpr D3D12_INPUT_ELEMENT_DESC inputLayout[] = {
		{ "POSITION",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{"TEXCOORD",0,DXGI_FORMAT_R32G32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0},
		{ "NORMAL",0,DXGI_FORMAT_R32G32B32_FLOAT, 0, D3D12_APPEND_ALIGNED_ELEMENT, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA,0}
	};

	static constexpr VertexAttributeDesc attributeData[] = {
		{AttributeSemantic::Value::Position, AttributeType::Value::Float32, 3U, 0U, 4U},
		{AttributeSemantic::Value::Texture, AttributeType::Value::Float32, 2U, 12U, 4U},
		{AttributeSemantic::Value::Normal, AttributeType::Value::Float32, 3U, 20U, 4U}
	};

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
	static constexpr VertexAttributeDesc attributeData[] = {
		{AttributeSemantic::Value::Position, AttributeType::Value::Float32, 3U, 0U, 4U}
	};

	VertexPos() = default;
	VertexPos(const DirectX::XMFLOAT3 &v) :
		vert(v)
	{}
};

namespace VertexTypes{
	template <typename T>
	struct VertexTraits;

	template<>
	struct VertexTraits<VertexPosTexNorm>{
		static bool matches(const VertexLayout &layout){
			return (layout.stride==sizeof(VertexPosTexNorm))&&
				layout.has(AttributeSemantic::Value::Position, 3)&&
				layout.has(AttributeSemantic::Value::Texture, 2)&&
				layout.has(AttributeSemantic::Value::Normal, 3);
		}
	};

	template<>
	struct VertexTraits<VertexPos>{
		static bool matches(const VertexLayout &layout){
			return (layout.stride==sizeof(VertexPos))&&
				layout.has(AttributeSemantic::Value::Position, 3);
		}
	};


	template <typename T>
	bool matches(const VertexLayout &layout){
		static_assert(sizeof(VertexTraits<T>)>0, "No VertexTrait specialization for this type");
		return VertexTraits<T>::matches(layout);
	}
}
