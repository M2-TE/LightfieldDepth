#define BRIGHTNESS(col) dot(col, float3(0.333333f, 0.333333f, 0.333333f)); // using standard greyscale
#define BRIGHTNESS_REAL(col) dot(col, float3(0.299f, 0.587f, 0.114f)); // using luminance construction

// mip-map/pyramid
// closer far plane for cam matrix
// how to 1D differentiator -> 2D differentiator? DFT bildbearbeitung sources?
// another keyword: separable kernels?
// matlab fdatool for 1D filters
// library genesis .ru

Texture2DArray colBuffArr : register(t0);
Texture2DArray colBuffArr2 : register(t1);

float2 get_disparity(int3 texPos, int tapSize, float p[9], float d[9])
{
    // cam-specific filters
    float p_tap3[] = {  0.229879f, 0.540242f, 0.229879f };
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
    
    // get disparity and confidence
    float a = Lx * Lu + Ly * Lv;
    float confidence = Lx * Lx + Ly * Ly;
    float disparity = a / confidence;
    return float2(disparity, confidence);
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

    // get disparity for current pixel using given filters (x is disparity s, y is confidence value)
    float2 s_3 = get_disparity(texPos, 3, p_tap3, d_tap3);
    float2 s_5 = get_disparity(texPos, 5, p_tap5, d_tap5);
    float2 s_7 = get_disparity(texPos, 7, p_tap7, d_tap7);
    float2 s_9 = get_disparity(texPos, 9, p_tap9, d_tap9);
    
    float s = s_3.x * s_3.y + s_5.x * s_5.y + s_7.x * s_7.y + s_9.x * s_9.y;
    float totalConfidence = s_3.y + s_5.y + s_7.y + s_9.y;
    s /= totalConfidence;
    
    // derive depth from disparity (barebones)
    float modA = 0.0f, modB = 10000000.0f;
    float depth = 1.0f / (modA + modB * s_9.y);

    return float4(depth, depth, depth, 1.0f);
}