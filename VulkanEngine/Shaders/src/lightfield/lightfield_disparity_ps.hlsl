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

float convert_back(float s)
{
    return s * 20.0f - 10.0f;
}

float4 read_color(uint2 pos)
{
    if (pcs.iRenderMode == 3)
    {
        float f = colorTex[pos].r;
        return get_heat(convert_back(f));
    }
    
    if (pcs.iRenderMode == 7)
    {
        float f = colorTex[pos].r;
        return convert_back(f).rrrr;
    }
    else
    {
        return colorTex[pos];
    }
}

// basically post processing
float4 main(float4 screenPos : SV_Position) : SV_Target
{
    // TODO: 
    // gauss blur
    // general average
    // (haar wavelet)
    if (pcs.iRenderMode == 6)
    {
        float comparison = comparisonTex[uint2(screenPos.x, screenPos.y)];
        return get_heat(comparison);
    }
    else
    {
        float4 color;
        color.w = 1.0f;
        if (pcs.iPostProcessingMode == 0) // no processing
        {
            color = read_color(uint2(screenPos.x, screenPos.y));
        }
        else if (pcs.iPostProcessingMode == 1) // 3x3 average blur
        {
            float3 val = float3(0.0f, 0.0f, 0.0f);
            for (int x = -1; x < 2; x++) {
                for (int y = -1; y < 2; y++) {
                    val += read_color(uint2(screenPos.x + x, screenPos.y + y)).xyz;
                }
            }
            color.xyz = val / 9.0f;
        }
        else // 3x3 gauss blur
        {
            float gauss[9] =
            {
                1.0f / 16.0f, 1.0f / 8.0f, 1.0f / 16.0f,
                1.0f / 8.0f, 1.0f / 4.0f, 1.0f / 8.0f,
                1.0f / 16.0f, 1.0f / 8.0f, 1.0f / 16.0f
                
                //0.075f, 0.124f, 0.075f,
                //0.124f, 0.204f, 0.124f,
                //0.075f, 0.124f, 0.075f
            };
            
            float3 val = float3(0.0f, 0.0f, 0.0f);
            for (int x = -1; x < 2; x++)
            {
                for (int y = -1; y < 2; y++)
                {
                    float gaussFactor = gauss[(x + 1) + (y + 1) * 3];
                    val += read_color(uint2(screenPos.x + x, screenPos.y + y)).xyz * gaussFactor;
                }
            }
            color.xyz = val;
            
        }
        
        if (pcs.iRenderMode == 7) // comparison mode, comparing approximated disparity with approximated depth
        {
            float comparison = comparisonTex[uint2(screenPos.x, screenPos.y)];
            //float disp = read_color(uint2(screenPos.x, screenPos.y)).r;
            float disp = color.x;
        //return get_heat(comparison - disp);
            return (comparison - disp) * (comparison - disp);
        }
        else
        {
            return color;
        }
    }
}