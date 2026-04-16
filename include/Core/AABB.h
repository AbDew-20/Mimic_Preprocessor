#pragma once
#include <vector>
#include <Core/VertexTypes.h>
#include <DirectXMath.h>



struct AABB{
	DirectX::XMFLOAT3 max;
	DirectX::XMFLOAT3 min;

	AABB() = default;

	float GetAABBSurfaceArea() const{
		using namespace DirectX;
		float area = 0.0f;
		XMFLOAT3 diff;
		XMStoreFloat3(&diff, XMVectorSubtract(XMLoadFloat3(&max), XMLoadFloat3(&min)));
		area += (2*diff.x*diff.y);
		area += (2*diff.z*diff.y);
		area += (2*diff.x*diff.z);
		
		return area;
	}

	float GetAABBVolume() const{
		using namespace DirectX;
		XMFLOAT3 diff;
		XMStoreFloat3(&diff, XMVectorSubtract(XMLoadFloat3(&max), XMLoadFloat3(&min)));
		float vol= diff.x*diff.y*diff.z;
		return vol;
	}

	void GetCenter(DirectX::XMFLOAT3 &center) const{
		using namespace DirectX;
		DirectX::XMStoreFloat3(&center, DirectX::XMVectorLerp(XMLoadFloat3(&min),XMLoadFloat3(&max), 0.5f));
	}

	void GetVertices(std::vector<VertexPos> &verts) const{
		verts.resize(8);
		const uint8_t xMask = 0b00000001;
		const uint8_t yMask = 0b00000010;
		const uint8_t zMask = 0b00000100;
		for(uint8_t j = 0; j<8; ++j){
			VertexPos vert;
			vert.vert.x = (j&xMask) ? max.x : min.x;
			vert.vert.y = (j&yMask) ? max.y : min.y;
			vert.vert.z = (j&zMask) ? max.z : min.z;
			verts[j]=vert;
		}
	}

	float GetDiagonalLength()const{
		using namespace DirectX;
		XMFLOAT3 len;
		XMStoreFloat3(&len, XMVector3Length(XMVectorSubtract(XMLoadFloat3(&max), XMLoadFloat3(&min))));
		return len.x;
	}
};
