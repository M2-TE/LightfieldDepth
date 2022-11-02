#define BRIGHTNESS(col) dot(col, float3(0.333333f, 0.333333f, 0.333333f)); // using standard greyscale
//#define BRIGHTNESS(col) dot(col, float3(0.299f, 0.587f, 0.114f)); // using luminance construction

// anisotropic filtering? - UNFIT (static filter size, would potentially include false depths)
// fix imag loading thingy - DONE
// depth factor sliders - DONE
// substitute 3x3 by 5x5 if 0.0f - DONE
//      -> blur results to combine diff filter sizes? - NOT NEEDED

// fill algorithm for 0.0f spots - IN PROGRESS

// comparison to ideal depth/disparity
// haar wavelet? transformation (blur instead of gauss) (kinda like fourier)

// EXTRA:
// variable camera array size/shape
// ui interaction for post processing + filter sizes

struct PCS
{
    uint index;
    float depthModA; 
    float depthModB;
    bool bGradientFillers;
};
[[vk::push_constant]] PCS pcs;
Texture2DArray colBuffArr : register(t0);

float4 get_gradients(int3 texPos, int tapSize, float p[9], float d[9])
{
    // cam-specific filters
    float p_tap3[] = { 0.229879f, 0.540242f, 0.229879f };
    float d_tap3[] = { -0.425287f, 0.000000f, 0.425287f };
    
    // lightfield derivatives
    float Lx = 0.0f, Ly = 0.0f;
    float Lu = 0.0f, Lv = 0.0f;

    int nCams = 3; // cams in one dimension
    int nPixels = tapSize; // pixels in one dimension
    int pixelOffset = nPixels / 2;
    
    // iterate over 2D patch of pixels (3x3)
    for (int x = 0; x < nPixels; x++)
    {
        for (int y = 0; y < nPixels; y++)
        {
            for (int u = 0; u < nCams; u++)
            {
                for (int v = 0; v < nCams; v++)
                {
                    int camIndex = u * 3 + v;
                    int3 texOffset = int3(x - pixelOffset, y - pixelOffset, camIndex);

                    float3 color = colBuffArr[uint3(texPos + texOffset)].rgb;
                    float luma = BRIGHTNESS(color);
                    
                    // approximate derivatives using 3-tap filter
                    Lx += d[x] * p[y] * p_tap3[u] * p_tap3[v] * luma;
                    Ly += p[x] * d[y] * p_tap3[u] * p_tap3[v] * luma;
                    Lu += p[x] * p[y] * d_tap3[u] * p_tap3[v] * luma;
                    Lv += p[x] * p[y] * p_tap3[u] * d_tap3[v] * luma;
                }
            }
        }
    }
    
    return float4(Lx, Ly, Lu, Lv);
}
float2 get_disparity(float4 gradients)
{
    // get disparity and confidence
    float a = gradients.x * gradients.z + gradients.y * gradients.w;
    float confidence = gradients.x * gradients.x + gradients.y * gradients.y;
    float disparity = a / confidence;
    return float2(disparity, confidence);
}
float4 get_heat(float val)
{
    // heatmap view (https://www.shadertoy.com/view/WslGRN)
    float heatLvl = val * 3.14159265 / 2;
    return float4(sin(heatLvl), sin(heatLvl * 2), cos(heatLvl), 1.0f);
}

float4 main(float4 screenPos : SV_Position) : SV_Target
{
    int3 texPos = int3(screenPos.xy, 0);
    
    // derivative approximation filters
    float p_tap3[9] = {  0.229879f,  0.540242f,  0.229879f,/**/0.000000f, 0.000000f,    0.000000f, 0.000000f,    0.000000f, 0.000000f };
    float d_tap3[9] = { -0.425287f,  0.000000f,  0.425287f,/**/0.000000f, 0.000000f,    0.000000f, 0.000000f,    0.000000f, 0.000000f };
    float p_tap5[9] = {  0.037659f,  0.249153f,  0.426375f,    0.249153f, 0.037659f,/**/0.000000f, 0.000000f,    0.000000f, 0.000000f };
    float d_tap5[9] = { -0.109604f, -0.276691f,  0.000000f,    0.276691f, 0.109604f,/**/0.000000f, 0.000000f,    0.000000f, 0.000000f };
    float p_tap7[9] = {  0.004711f,  0.069321f,  0.245410f,    0.361117f, 0.245410f,    0.069321f, 0.004711f,/**/0.000000f, 0.000000f };
    float d_tap7[9] = { -0.018708f, -0.125376f, -0.193091f,    0.000000f, 0.193091f,    0.125376f, 0.018708f,/**/0.000000f, 0.000000f };
    float p_tap9[9] = {  0.000721f,  0.015486f,  0.090341f,    0.234494f, 0.317916f,    0.234494f, 0.090341f,    0.015486f, 0.000721f };
    float d_tap9[9] = { -0.003059f, -0.035187f, -0.118739f,   -0.143928f, 0.000000f,    0.143928f, 0.118739f,    0.035187f, 0.003059f };

    // use 3-tap filter to obtain gradients
    float4 gradients3 = get_gradients(texPos, 3, p_tap3, d_tap3);
    float4 gradients5 = get_gradients(texPos, 5, p_tap5, d_tap5);
    float4 gradients7 = get_gradients(texPos, 7, p_tap7, d_tap7);
    float4 gradients9 = get_gradients(texPos, 9, p_tap9, d_tap9);
    
    float4 gradients;
    // choose gradients
    if (pcs.bGradientFillers)
    {
        gradients = gradients3;
        // when gradients equal 0, choose larger filter size
        float cutoff = 0.000001f;
        gradients =
            gradients3 > cutoff || gradients3 < -cutoff ? gradients3 :
            gradients5 > cutoff || gradients5 < -cutoff ? gradients5 :
            gradients7 > cutoff || gradients7 < -cutoff ? gradients7 :
            gradients9 > cutoff || gradients9 < -cutoff ? gradients9 : gradients3;
    }
    else
    {
        gradients = gradients3;
    }
    
    // get disparity for current pixel using given filters (x is disparity, y is confidence value)
    float2 disparity = get_disparity(gradients);
    
    
    float4 col = float4(0.0f, 0.0f, 0.0f, 1.0f);
    if (pcs.index == 0) // middle view
    {
        return colBuffArr[uint3(screenPos.xy, 4)];
    }
    else if (pcs.index == 1) // gradients view (Lx & Lu)
    {
        return float4(gradients.x, gradients.z, 0.0f, 1.0f);
    }
    else if (pcs.index == 2) // gradients view (Ly, Lv)
    {
        return float4(gradients.y, gradients.w, 0.0f, 1.0f);
    }
    else if (pcs.index == 3) // disparity view
    {
        float s = disparity.x;
        //col = float4(s, s, s, 1.0f);
        
        return get_heat(s);
    }
    else if (pcs.index == 4) // depth view
    {
        // derive depth from disparity
        float depth = 1.0f / (pcs.depthModA + pcs.depthModB * disparity.x);
        //col = float4(depth, depth, depth, 1.0f);
        
        return get_heat(depth);
    }
    else // test filler view
    {
        float cutoff = 0.000001f;
        gradients =
            gradients3 > cutoff || gradients3 < -cutoff ? float4(0.0f, 0.0f, 0.0f, 1.0f) :
            gradients5 > cutoff || gradients5 < -cutoff ? float4(1.0f, 0.0f, 0.0f, 1.0f) :
            gradients7 > cutoff || gradients7 < -cutoff ? float4(0.0f, 1.0f, 0.0f, 1.0f) :
            gradients9 > cutoff || gradients9 < -cutoff ? float4(0.0f, 1.0f, 0.0f, 1.0f) : float4(1.0f, 1.0f, 1.0f, 1.0f);
        
        return gradients;
    }
}