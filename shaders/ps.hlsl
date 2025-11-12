struct PSInput
{
    float4 position : SV_Position;
    float4 normal :NORMAL;
};

struct PSOutput
{
    float4 colour : SV_Target;
};

static const float3 lightDir = normalize(float3(1.0f, -1.0f, 0.0f));
static const float ambient = 0.2f;

 PSOutput main(PSInput input)
{
    PSOutput output;
    float3 pixelNormal = input.normal.rgb;
    float3 albedo = float3(1.0f,0.0f,0.0f);
    float diffuse = saturate(dot(pixelNormal, -lightDir))  + ambient;
    output.colour = saturate(float4(mul(diffuse, albedo), 1.0));

    
    return output;
}