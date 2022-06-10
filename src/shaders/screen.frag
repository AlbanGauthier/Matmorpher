#version 450 core

layout(location = 0) in vec3 frag_position;
layout(location = 1) in vec2 frag_texcoord;

layout(binding = 0) uniform sampler2D screenTexture;

uniform int debug_view_mode;
out vec4 out_color;

const float gamma = 2.2;
const float inv_gamma = 1.0 / gamma;

// #####################
// #### ToneMappers ####
// #####################

// linear to sRGB approximation
// see http://chilliant.blogspot.com/2012/08/srgb-approximations-for-hlsl.html
vec3 lineartosRGB(vec3 color)
{
    return pow(color, vec3(inv_gamma));
}

vec3 sRGBtoLinear(vec3 color)
{
    return pow(color, vec3(gamma));
}

vec3 simpleReinhardToneMapping(vec3 color)
{
	float exposure = 2.5;
    float gamma = 1.0;
    
	color *= exposure/(1. + color / exposure);
	color = pow(color, vec3(1. / gamma));
	return color;
}

vec3 filmicToneMapping(vec3 color)
{
	color = max(vec3(0.), color - vec3(0.004));
	color = (color * (6.2 * color + .5)) / (color * (6.2 * color + 1.7) + 0.06);
	return color;
}

// Uncharted 2 tone map
// see: http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 toneMapUncharted2Impl(vec3 color)
{
    const float A = 0.15;
    const float B = 0.50;
    const float C = 0.10;
    const float D = 0.20;
    const float E = 0.02;
    const float F = 0.30;
    return ((color*(A*color+C*B)+D*E)/(color*(A*color+B)+D*F))-E/F;
}

vec3 toneMapUncharted(vec3 color)
{
    const float W = 11.2;
    color = toneMapUncharted2Impl(color * 2.0);
    vec3 whiteScale = 1.0 / toneMapUncharted2Impl(vec3(W));
    return lineartosRGB(color * whiteScale);
}

// Hejl Richard tone map
// see: http://filmicworlds.com/blog/filmic-tonemapping-operators/
vec3 toneMapHejlRichard(vec3 color)
{
    color = max(vec3(0.0), color - vec3(0.004));
    return (color*(6.2*color+.5))/(color*(6.2*color+1.7)+0.06);
}

void main()
{
    vec3 color = texelFetch(screenTexture, ivec2(gl_FragCoord.xy), 0).rgb;

    if(debug_view_mode == 0 || debug_view_mode == 9)
    {
        // color = lineartosRGB(color);
        // color = toneMapHejlRichard(color);
        // color = toneMapUncharted(color);
        color = simpleReinhardToneMapping(color);
        // color = filmicToneMapping(color);
    }

    out_color = vec4(color, 1.0);
}