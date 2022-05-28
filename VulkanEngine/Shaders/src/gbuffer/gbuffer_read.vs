float4 main(uint vertID : SV_VertexID) : SV_Position
{
    //float2 uv = float2((vertID << 1) & 2, vertID & 2);
    return float4((vertID == 1) ? 3.0f : -1.0f, (vertID == 2) ? -3.0f : 1.0f, 1.0f, 1.0f);
}