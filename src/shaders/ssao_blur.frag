#version 450 core

in vec2 frag_texcoord;

layout(binding = 0) uniform sampler2D SSAOBuffer;

out float out_color;

void main()
{
    vec2 texelSize = 1.0 / vec2(textureSize(SSAOBuffer, 0));
    
    float result = 0.0;
    
    for (int x = -2; x < 2; ++x) 
    {
        for (int y = -2; y < 2; ++y) 
        {
            vec2 offset = vec2(float(x), float(y)) * texelSize;
            result += texture(SSAOBuffer, frag_texcoord + offset).r;
        }
    }

    out_color = result / (4.0 * 4.0);
}