struct Input
{
    float4 position : Position;
    float4 color : Color;
};

struct Output
{
    float4 screenPos : SV_Position;
    float4 color : Color;
};

cbuffer MVP : register(b0)
{
    float4x4 model;
    float4x4 view;
    float4x4 proj;
};

Output main(Input input)
{
    Output output;
    output.screenPos = mul(model, input.position);
    output.screenPos = mul(view, output.screenPos);
    output.screenPos = mul(proj, output.screenPos);
    output.color = input.color;
    return output;
}