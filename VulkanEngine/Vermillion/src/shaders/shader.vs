struct Input
{
    uint vertID : SV_VertexID;
};

struct Output
{
    float4 color : Color;
    float4 screenPos : SV_Position;
};

static const float2 positions[3] =
{
    float2(0.0f, -0.5f),
    float2(0.5f, 0.5f),
    float2(-0.5f, 0.5f)
};

static const float3 colors[3] =
{
    float3(1.0f, 0.0f, 0.0f),
    float3(0.0f, 1.0f, 0.0f),
    float3(0.0f, 0.0f, 1.0f)
};

Output main(Input input)
{
    Output output;
    output.color = float4(colors[input.vertID], 1.0f);
    output.screenPos = float4(positions[input.vertID], 0.0f, 1.0f);
    return output;
}