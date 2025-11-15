struct ModelViewProjection
{
    matrix MVP;
    matrix model;
};


ConstantBuffer<ModelViewProjection> ModelViewProjectionCB : register(b0);


struct VertexData
{
    float3 position :POSITION;
};

struct VSOutput
{
    float4 position : SV_Position;
};


VSOutput main(VertexData input)
{
    VSOutput output;
    output.position = mul(ModelViewProjectionCB.MVP, float4(input.position, 1.0f));

    return output;

}
