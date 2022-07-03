struct Input
{
    float4 position : Position;
    float4 color : Color;
    float4 normal : Normal;
};

struct Output
{
    float4 screenPos : SV_Position;
    float4 worldPos : WorldPos;
    float4 color : Color;
    float4 normal : Normal;
};

[[vk::binding(0, 0)]] // binding slot 0, descriptor set 0
cbuffer ModelBuffer { float4x4 model; };
[[vk::binding(1, 0)]] // binding slot 1, descriptor set 0
cbuffer ViewProjectionBuffer { float4x4 view, proj, viewProj; };


Output main(Input input)
{
    Output output;
    output.worldPos = input.position;
    output.screenPos = mul(viewProj, output.worldPos);
    output.color = input.color;
    output.normal = input.normal;
    return output;
}