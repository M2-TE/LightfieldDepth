struct PCS
{
    uint iRenderMode;
    uint iFilterMode;
    uint iPostProcessingMode;
    
    float depthModA;
    float depthModB;
    
    uint bUseHeat;
};
[[vk::push_constant]] PCS pcs;
Texture2D colorTex : register(t0);
Texture2D<float> comparisonTex : register(t1);

float4 get_heat(float val)
{
    // heatmap view (https://www.shadertoy.com/view/WslGRN)
    float heatLvl = val * 3.14159265 / 2;
    return float4(sin(heatLvl), sin(heatLvl * 2), cos(heatLvl), 1.0f);
}

// basically post processing
float4 main(float4 screenPos : SV_Position) : SV_Target
{
    // TODO: 
    // gauss blur
    // general average
    // haar wavelet
    if (pcs.iRenderMode == 6)
    {
        float comparison = comparisonTex[uint2(screenPos.x, screenPos.y)];
        float4 color = get_heat(comparison);
        return color;
    }
    else
    {
        float4 color = colorTex[uint2(screenPos.x, screenPos.y)];
        return color;
    }
}