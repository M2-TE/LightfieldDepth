[[vk::input_attachment_index(0)]] SubpassInput gradientBuffer;

// this shader pass isnt even necessary anymore
float4 main() : SV_Target
{
    return gradientBuffer.SubpassLoad();
}