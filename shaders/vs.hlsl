struct ModelViewProjection
{
    matrix MVP;
    matrix model;
};


ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);


struct VertexData
{
    float3 position :POSITION;
    float2 texCoord :TEXCOORD;
    float3 normal :NORMAL;
};

struct VSOutput
{
    float4 position : SV_Position;
    float4 normal : NORMAL;
};


VSOutput main(VertexData input)
{
    VSOutput output;
    output.position = mul(ModelViewProjectionCB.MVP, float4(input.position, 1.0f));
    output.normal = mul(ModelViewProjectionCB.model, float4(input.normal, 1.0f));

    return output;

}