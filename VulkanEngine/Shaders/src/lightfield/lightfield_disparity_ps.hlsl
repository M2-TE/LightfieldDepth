[[vk::input_attachment_index(0)]] SubpassInput gradientBuffer;

// this shader pass isnt even necessary anymore
float4 main() : SV_Target
{
    float4 gradients = gradientBuffer.SubpassLoad();
    float a = gradients.x * gradients.z + gradients.y * gradients.w;
    float b = gradients.x * gradients.x + gradients.y * gradients.y;
    
    float depth = a / b;
    depth /= 10.0f; // scaling down disparity to normalize between 0 and 1
    
    float confidence = b;
    confidence *= 10000.0f; // scaling up confidence estimate so that it can actually be represented visually
    
    // filter out confidence values which are practically 0 (could switch to another filter size in this case?)
    if (confidence < 0.00001f) confidence = 1.0f;
    
    depth = confidence; // show confidence instead of depth;
    
    return float4(depth, depth, depth, 1.0f);
}