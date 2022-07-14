[[vk::input_attachment_index(0)]] SubpassInput gradientBuffer;

float4 main() : SV_Target
{
    float4 gradients = gradientBuffer.SubpassLoad();
    float a = gradients.x * gradients.z + gradients.y * gradients.w;
    float b = gradients.x * gradients.x + gradients.y * gradients.y;
        
    float depth = a / b;
    depth /= 10.0f; // scaling down disparity to normalize between 0 and 1
    return float4(depth, depth, depth, 1.0f);
}