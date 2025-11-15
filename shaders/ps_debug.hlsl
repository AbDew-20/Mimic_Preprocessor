struct PSInput
{
    float4 position : SV_Position;
};

struct PSOutput
{
    float4 colour : SV_Target;
};

static const float4 wireFrameColour = {0.0f,1.0f,0.0f,1.0f };

 PSOutput main(PSInput input)
{
    PSOutput output;
    output.colour = wireFrameColour;
    
    return output;
}
