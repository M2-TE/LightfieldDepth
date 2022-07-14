struct Input
{
    float4 worldPos : WorldPos;
    float4 color : Color;
    float4 normal : Normal;
};

float4 main(Input input) : SV_Target
{
    float3 lightPos = float3(0.0f, 0.0f, -10.0f); // hard coded point light
    float3 dir = lightPos - input.worldPos.xyz;
    float dist = length(dir);
    float attenuation = 1.0f / pow(dist, 0.3f);
    //float attenuation = 10.0f / (dist * dist);
    float3 col = input.color.rgb * max(dot(normalize(dir), input.normal.xyz) * attenuation, 0.05f);

    // another option for a more controlled attentuation curve (maybe for later)
    //float a, b, c;
    //float attenB = 1.0f / (a * dist * dist + b * dist + c);
    
    return float4(col, input.color.a);
}