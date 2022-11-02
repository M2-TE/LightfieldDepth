//[[vk::input_attachment_index(0)]] SubpassInput gradientBuffer;
Texture2D colorTex : register(t0);

// basically post processing
float4 main(float4 screenPos : SV_Position) : SV_Target
{
    // TODO: 
    // gauss blur
    // general average
    // haar wavelet
    
    float4 color = colorTex[uint2(screenPos.x, screenPos.y)];
    return color;
}