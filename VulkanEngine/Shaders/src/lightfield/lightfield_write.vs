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

[[vk::binding(2, 1)]] // binding slot 2, descriptor set 1
cbuffer OffsetBuffer { float4 posOffset; };


Output main(Input input)
{
    Output output;
    output.worldPos = input.position;
    output.worldPos = mul(view, output.worldPos);
    output.worldPos += posOffset; // individual cam offset
    
    output.screenPos = mul(proj, output.worldPos);
    output.color = input.color;
    output.normal = input.normal;
    return output;
}