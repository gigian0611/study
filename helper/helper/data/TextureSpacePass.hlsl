
struct VSInput
{
    float3 position : POSITION;
    float3 normal   : NORMAL;
    float2 texcoord : TEXCOORD;
};

struct PSInput
{
    float4 positionClip : SV_POSITION;
    float3 position     : POSITION;
    float3 normal       : NORMAL;
    float2 texcoord     : TEXCOORD;
};

cbuffer cb0 : register(b0)
{
    row_major float4x4 M;
};

Texture2D diffuseColor : register(t0);
SamplerState sampler0 : register(s0);


PSInput VSMain(VSInput input)
{
    PSInput result;
    result.positionClip     = float4(input.texcoord * float2(2.0,-2.0) + float2(-1.0,1.0), 0.0, 1.0);
    result.position         = mul(float4(input.position, 1.0), M).xyz;
    result.normal           = mul(float4(input.normal, 0.0), M).xyz;
    result.texcoord         = input.texcoord;
    return result;
}

void PSMain(
    PSInput input,
    out float4 outTarget0 : SV_TARGET0,
    out float4 outTarget1 : SV_TARGET1,
    out float4 outTarget2 : SV_TARGET2
)
{
    outTarget0 = diffuseColor.Sample(sampler0, input.texcoord);
    outTarget1 = float4(input.position.xyz, 1.0);
    outTarget2 = float4(normalize(input.normal), 1.0);
}
