
static const float4 uniformRect[4] = {
        float4(-0.5, -0.5, 0, 1),
        float4( 0.5, -0.5, 0, 1),
        float4(-0.5,  0.5, 0, 1),
        float4( 0.5,  0.5, 0, 1)
};

cbuffer ViewData : register(b0)
{
    row_major float4x4 MVP;
    float4 color;
};

struct PSInput
{
    float4 clipPos  : SV_POSITION;
    float4 color    : COLOR;
};

PSInput VSMain(in uint vId : SV_VertexID)
{
    PSInput result;
    result.clipPos = mul(uniformRect[vId], MVP);
    result.color = color;
    return result;
}

float4 PSMain(PSInput input) : SV_TARGET0
{
    return input.color;
}
