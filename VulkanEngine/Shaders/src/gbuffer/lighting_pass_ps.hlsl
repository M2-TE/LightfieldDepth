[[vk::input_attachment_index(0)]] SubpassInput gPos;
[[vk::input_attachment_index(1)]] SubpassInput gCol;
[[vk::input_attachment_index(2)]] SubpassInput gNorm;

float4 main() : SV_Target
{
    float4 pos = gPos.SubpassLoad();
    float4 col = gCol.SubpassLoad();
    float4 normal = gNorm.SubpassLoad();
    
    float3 lightPos = float3(5.0f, -5.0f, -5.0f); // hard coded point light
    float3 dir = lightPos - pos.xyz;
    float dist = length(dir);
    float attenuation = 30.0f / (dist * dist);
    col.rgb *= max(dot(normalize(dir), normal.xyz) * attenuation, 0.1f);

    // another option for a more controlled attentuation curve (maybe for later)
    float a, b, c;
    float attenB = 1.0f / (a + b * dist + c * dist * dist);
    
    return float4(col);
}