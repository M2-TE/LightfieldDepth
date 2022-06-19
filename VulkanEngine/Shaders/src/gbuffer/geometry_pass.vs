struct Input
{
    float4 position : Position;
    float4 color : Color;
};

struct Output
{
    float4 screenPos : SV_Position;
    float4 worldPos : WorldPos;
    float4 color : Color;
};

[[vk::binding(0, 0)]] // binding slot 0, descriptor set 0
cbuffer ModelBuffer
{
    float4x4 model;
};
[[vk::binding(1, 0)]] // binding slot 1, descriptor set 0
cbuffer ViewProjectionBuffer
{
    float4x4 view;
    float4x4 proj;
    float4x4 viewProj;
};


Output main(Input input)
{
    Output output;
    output.worldPos = input.position;
    output.screenPos = mul(viewProj, output.worldPos);
    output.color = input.color;
    return output;
}