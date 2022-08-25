#define BRIGHTNESS(col) dot(col, float3(0.333333f, 0.333333f, 0.333333f)); // using standard greyscale
#define BRIGHTNESS_REAL(col) dot(col, float3(0.299f, 0.587f, 0.114f)); // using luminance construction

// mip-map/pyramid
// closer far plane for cam matrix
// how to 1D differentiator -> 2D differentiator? DFT bildbearbeitung sources?
// another keyword: separable kernels?
// matlab fdatool for 1D filters
// library genesis .ru

Texture2DArray colBuffArr : register(t0);

float4 main(float4 screenPos : SV_Position) : SV_Target
{
    const int3 texPos = int3(screenPos.xy, 0);

    // lightfield derivatives
    float Lx = 0.0f;
    float Ly = 0.0f;
    float Lu = 0.0f;
    float Lv = 0.0f;

    // derivative approximation filters
    float p_tap3[] = {  0.229879f, 0.540242f, 0.229879f };
    float d_tap3[] = { -0.425287f, 0.000000f, 0.425287f };
    float p_tap5[] = {  0.037659f,  0.249153f, 0.426375f, 0.249153f, 0.037659f };
    float d_tap5[] = { -0.109604f, -0.276691f, 0.000000f, 0.276691f, 0.109604f };
    float p_tap7[] = {  0.004711f,  0.069321f,  0.245410f, 0.361117f, 0.245410f, 0.069321f, 0.004711f };
    float d_tap7[] = { -0.018708f, -0.125376f, -0.193091f, 0.000000f, 0.193091f, 0.125376f, 0.018708f };
    float p_tap9[] = {  0.000721f,  0.015486f,  0.090341f,  0.234494f, 0.317916f, 0.234494f, 0.090341f, 0.015486f, 0.000721f };
    float d_tap9[] = { -0.003059f, -0.035187f, -0.118739f, -0.143928f, 0.000000f, 0.143928f, 0.118739f, 0.035187f, 0.003059f };
    
    int nCams = 3; // cams in one dimension
    int nPixels = 3; // pixels in one dimension
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
                    Lx += d_tap3[x] * p_tap3[y] * p_tap3[u] * p_tap3[v] * luma;
                    Ly += p_tap3[x] * d_tap3[y] * p_tap3[u] * p_tap3[v] * luma;
                    Lu += p_tap3[x] * p_tap3[y] * d_tap3[u] * p_tap3[v] * luma;
                    Lv += p_tap3[x] * p_tap3[y] * p_tap3[u] * d_tap3[v] * luma;
                }
            }
        }
    }
    
    // <> //

    return float4(Lx, Ly, Lu, Lv);
}