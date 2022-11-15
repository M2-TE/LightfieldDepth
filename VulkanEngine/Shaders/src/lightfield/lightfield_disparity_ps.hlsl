Texture2D colorTex : register(t0);
Texture2D comparisonTex : register(t1);

// basically post processing
float4 main(float4 screenPos : SV_Position) : SV_Target
{
    // TODO: 
    // gauss blur
    // general average
    // haar wavelet
    
    float4 color = comparisonTex[uint2(screenPos.x, screenPos.y)];
    //float4 color = colorTex[uint2(screenPos.x, screenPos.y)];
    return color;
}