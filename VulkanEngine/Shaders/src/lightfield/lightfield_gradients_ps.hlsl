#define BRIGHTNESS(col) dot(col, float3(0.333333f, 0.333333f, 0.333333f)); // using standard greyscale
//#define BRIGHTNESS(col) dot(col, float3(0.299f, 0.587f, 0.114f)); // using luminance construction

// anisotropic filtering? - UNFIT (static filter size, would potentially include false depths)

// friday:
// swich between filters using buttons // DONE
// certainty view // DONE
// use certainty instead of 0.0 check // DONE
//  -> scaling certainty to make it visible // DONE

// saturday:
// fix tex read error on post processing // DONE
// simulated model view with button // DONE
// make sphere have colored surface? // DONE
// change the color algorithm with sine wave? // found something better

// sunday:
// variable camera array size/shape or distance // DONE
//  -> offset can now be varied using UI element
//  -> selecting specific cameras at runtime is a bit difficult atm, 
//     opting to just hardcode them instead just to test the results

// monday:
// mean squared error/sum of absolute differences for comparison
// comparison to ideal depth/disparity
// why the hell are they using .pfm file formats for ground truth..
// -> its not even standard pfm, its just a grayscale..

// tuesday:
// fix linux implementation

// TODO: update ui with all the new features!
// general ui cleanup..

// extra:
// gauss blur as post processing
// haar wavelet? transformation (blur instead of gauss) (kinda like fourier)

struct PCS
{
    uint iRenderMode;
    uint iFilterMode;
    
    float depthModA; 
    float depthModB;
    
    uint bUseHeat;
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
                    bool3x3 enabledCams = {
                        true, true, true,
                        true, true, true,
                        true, true, true
                    };
                    if (!enabledCams[u][v]) continue;
                    
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
    float4x4 allGradients =
    {
        get_gradients(texPos, 3, p_tap3, d_tap3),
        get_gradients(texPos, 5, p_tap5, d_tap5),
        get_gradients(texPos, 7, p_tap7, d_tap7),
        get_gradients(texPos, 9, p_tap9, d_tap9)
    };
    float4x2 allDisparities =
    {
        get_disparity(allGradients[0]),
        get_disparity(allGradients[1]),
        get_disparity(allGradients[2]),
        get_disparity(allGradients[3])
    };
    
    float4 gradients;
    float disparity;
    float certainty;
    // choose gradients
    if (pcs.iFilterMode == 0)
    {
        // switching between gradients based on their values
        if (false)
        {
            // when gradients equal 0, choose larger filter size
            const float cutoff = 0.000001f;
            gradients =
                allGradients[0] > cutoff || allGradients[0] < -cutoff ? allGradients[0] :
                allGradients[1] > cutoff || allGradients[1] < -cutoff ? allGradients[1] :
                allGradients[2] > cutoff || allGradients[2] < -cutoff ? allGradients[2] :
                allGradients[3] > cutoff || allGradients[3] < -cutoff ? allGradients[3] : allGradients[0];
        }
        // choose the filter with the highest certainty
        else
        {
            uint baseIndex = 0;
            certainty = allDisparities[baseIndex].y;
            for (int i = 1; i < 4; i++)
            {
                baseIndex = certainty < allDisparities[i].y ? i : baseIndex;
                certainty = allDisparities[baseIndex].y;
            }
            gradients = allGradients[baseIndex];
            disparity = allDisparities[baseIndex].x;
            
        }
    }
    else // specific filter for gradients
    {
        uint index = pcs.iFilterMode - 1u;
        gradients = allGradients[index];
        disparity = allDisparities[index].x;
        certainty = allDisparities[index].y;
    }
    
    if (pcs.iRenderMode == 0) // middle view
    {
        return colBuffArr[uint3(screenPos.xy, 4)];
    }
    else if (pcs.iRenderMode == 1) // gradients view (Lx & Lu)
    {
        return float4(gradients.x, gradients.z, 0.0f, 1.0f);
    }
    else if (pcs.iRenderMode == 2) // gradients view (Ly, Lv)
    {
        return float4(gradients.y, gradients.w, 0.0f, 1.0f);
    }
    else if (pcs.iRenderMode == 3) // disparity view
    {
        float s = disparity.x;
        //col = float4(s, s, s, 1.0f);
        
        return get_heat(s);
    }
    else if (pcs.iRenderMode == 4) // depth view
    {
        // derive depth from disparity
        float depth = 1.0f / (pcs.depthModA + pcs.depthModB * disparity.x);
        //col = float4(depth, depth, depth, 1.0f);
        
        return get_heat(depth);
    }
    else if (pcs.iRenderMode == 5) // certainty view
    {
        // scale certainty to make it visible
        certainty *= 500.0f;
        return (get_heat(certainty));
    }
    else // test filler view
    {
        // when gradients equal 0, choose larger filter size
        const float cutoff = 0.000001f;
        gradients =
            allGradients[0] > cutoff || allGradients[0] < -cutoff ? float4(0.0f, 0.0f, 0.0f, 1.0f) :
            allGradients[1] > cutoff || allGradients[1] < -cutoff ? float4(1.0f, 0.0f, 0.0f, 1.0f) :
            allGradients[2] > cutoff || allGradients[2] < -cutoff ? float4(0.0f, 1.0f, 0.0f, 1.0f) :
            allGradients[3] > cutoff || allGradients[3] < -cutoff ? float4(0.0f, 0.0f, 1.0f, 1.0f) : float4(1.0f, 1.0f, 1.0f, 1.0f);
        
        return gradients;
    }
}