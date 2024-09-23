static const float4 QuadNDCoords[4] = {
        float4(-1, -1, 0, 1),
        float4(-1,  1, 0, 1),
        float4(1, -1, 0, 1),
        float4(1,  1, 0, 1)
};

static const float2 QuadUVCoords[4] = {
    float2(0, 1),
    float2(0, 0),
    float2(1, 1),
    float2(1, 0)
};

struct VSInput
{
    uint vId     : SV_VertexID;
};


struct PSInput
{
    float4 ndc  : SV_POSITION;
    float2 uv   : TEXCOORD;
};

cbuffer cb0 : register(b0)
{
    float4 position;
    float4 normal;
    float3 cameraPos;
    float intensity;
};

Texture2D diffuseMap : register(t0);
Texture2D positionMap : register(t1);
Texture2D normalMap : register(t2);

PSInput VSMain(VSInput input)
{
    PSInput result;
    result.ndc = QuadNDCoords[input.vId];
    result.uv = QuadUVCoords[input.vId];
    return result;
}

void PSMain(
    PSInput input,
    out float4 outTarget0 : SV_TARGET0
)
{
//15, 10, 1.0f

    
    float3 diffuseColor = diffuseMap.Load(uint3(input.ndc.x, input.ndc.y, 0)).rgb;
    float3 N = normalize(normalMap.Load(uint3(input.ndc.x, input.ndc.y, 0))).xyz;
    float3 P = positionMap.Load(uint3(input.ndc.x, input.ndc.y, 0)).xyz;
    //float3 V = normalize(cameraPos - P);
    float3 L = normalize(position - P);

    float dist = length(position - P);
    float cos_i = saturate(dot(N, L));
    float cos_j = saturate(dot(normal, -L));

    float3 v = normalize(P - cameraPos);
	float3 r = reflect(L,N);
    float cosAlpha = saturate(dot(v,r));

    float3 _diffuse = cos_i  * cos_j * diffuseColor * intensity/(dist*dist);
    float3 _speuclar = pow(cosAlpha,5)* float3(1,1,1) *intensity/(dist*dist);
    float3 _ambient = 0.2*diffuseColor;

    outTarget0 = float4(_diffuse + _speuclar + _ambient, 1);
    
    //if (cos_i < 0.0001)
    //    outTarget0 = float4(0, 0, 0, 1);
    
    
    
    //outTarget0 = float4(_diffuse, 1);
}
