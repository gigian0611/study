
struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct PSInput
{
    float4 positionClip : SV_POSITION;
    float2 texcoord     : TEXCOORD;
};

cbuffer cb0 : register(b0)
{
    row_major float4x4 MVP;
    row_major float4x4 MVP2;
};
Texture2D shadedColor : register(t0);
SamplerState sampler0 : register(s0);


PSInput VSMain(VSInput input)
{
    PSInput result;
    result.positionClip = mul(float4(input.position, 1.0), MVP);
    result.texcoord = input.texcoord;
    return result;
}

void PSMain(
    PSInput input,
    out float4 outTarget0 : SV_TARGET0
)
{
    outTarget0 = shadedColor.Sample(sampler0, input.texcoord);
}
