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

Output main(Input input)
{
    Output output;
    output.screenPos = input.position;
    output.color = input.color;
    return output;
}