#version 450 core

in vec2 frag_texcoord;

uniform float screen_height;
uniform float screen_width;

uniform mat4 projMatrix;
uniform mat4 projMatrixInv;

uniform mat4 viewMatrix;
uniform mat4 viewMatrixInv;

uniform float radius;

out float out_color;

layout(binding = 0) uniform sampler2D normalTexture;
layout(binding = 1) uniform sampler2D positionTexture;
layout(binding = 2) uniform sampler2D noiseTexture;

uniform vec3 samples[64];

// parameters (you'd probably want to use them as uniforms to more easily tweak the effect)
int kernelSize = 64;

float bias = 0.025;

float SSAO()
{
    vec3 normals = texture(normalTexture, frag_texcoord).xyz;
    vec4 view_normals = viewMatrix * vec4(normals, 1.0);
    
    // get input for SSAO algorithm
    vec3 fragPos = texture(positionTexture, frag_texcoord).xyz;
    vec3 normal = view_normals.xyz;
    vec3 randomVec = texture(noiseTexture, gl_FragCoord.xy / 4).xyz;
    
    // create TBN change-of-basis matrix: from tangent-space to view-space
    vec3 tangent = normalize(randomVec - normal * dot(randomVec, normal));
    vec3 bitangent = cross(normal, tangent);
    mat3 TBN = mat3(tangent, bitangent, normal);
    
    // iterate over the sample kernel and calculate occlusion factor
    float occlusion = 0.0;
    // int i = 0;
    for(int i = 0; i < kernelSize; ++i)
    {
        // get sample position
        vec3 sampleKernel = TBN * samples[i]; // from tangent to view-space
        sampleKernel = fragPos + sampleKernel * radius; 
        
        // project sample position (to sample texture) (to get position on screen/texture)
        vec4 offset = vec4(sampleKernel, 1.0);

        offset = projMatrix * offset; // from view to clip-space

        offset.xyz /= offset.w; // perspective divide
        offset.xyz = offset.xyz * 0.5 + 0.5; // transform to range 0.0 - 1.0

        if (gl_FragCoord.x < screen_width / 2.0f)
            offset.x = offset.x / 2.0f;
        else
            offset.x = offset.x / 2.0f + 0.5f;
        
        // get depth value of kernel sample
        vec3 samplePos = texture(positionTexture, offset.xy).xyz;

        // range check & accumulate
        float rangeCheck = smoothstep(0.0, 1.0, radius / abs(fragPos.z - samplePos.z));
        occlusion += (samplePos.z >= sampleKernel.z + bias ? 1.0 : 0.0) * rangeCheck;           
    }
    occlusion = 1.0 - (occlusion / kernelSize);

    return occlusion;
}

void main()
{
    vec3 fragPos = texture(positionTexture, frag_texcoord).xyz;
    vec3 randomVec = texture(noiseTexture, gl_FragCoord.xy / 4).xyz;
    vec3 normals = texture(normalTexture, frag_texcoord).rgb;
    vec4 view_normals = viewMatrix * vec4(normals, 1.0);

    // if(depth == 1.0)
    //     out_color = 1.0f;
    // else
        out_color = SSAO();
}