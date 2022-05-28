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
cbuffer MVP
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

Output main(Input input)
{
    Output output;
    output.worldPos = mul(model, input.position);
    output.screenPos = mul(view, output.worldPos);
    output.screenPos = mul(proj, output.screenPos);
    output.color = input.color;
    return output;
}