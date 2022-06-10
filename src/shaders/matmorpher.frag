#version 450 core

layout(location = 0) in vec3 worldPos;
layout(location = 1) in vec2 texcoord;
layout(location = 2) in vec3 normal;
layout(location = 3) in vec4 tangent;

layout (binding = 0) uniform sampler2D color_map;
layout (binding = 1) uniform sampler2D height_map;
layout (binding = 2) uniform sampler2D metallic_map;
layout (binding = 3) uniform sampler2D normal_map;
layout (binding = 4) uniform sampler2D roughness_map;
 
layout (binding = 5) uniform sampler2D color_map_2;
layout (binding = 6) uniform sampler2D height_map_2;
layout (binding = 7) uniform sampler2D metallic_map_2;
layout (binding = 8) uniform sampler2D normal_map_2;
layout (binding = 9) uniform sampler2D roughness_map_2;
 
layout (binding = 10) uniform sampler2D warpTexture;

layout (binding = 11) uniform sampler2D gaussianized_col1;
layout (binding = 12) uniform sampler2D gaussianized_col2;
 
layout (binding = 13) uniform sampler2D img1_inv_cdf; // 0-1024
layout (binding = 14) uniform sampler2D img2_inv_cdf; // 0-1024

layout (binding = 15) uniform sampler2D ssaoTexture;

//env map stored in float texture
layout(binding = 24) uniform sampler2D dfgLut;
layout(binding = 25) uniform samplerCube environmentMap;

uniform float mix_color;
uniform float mix_height;
uniform float mix_normal;
uniform float mix_metallic;
uniform float mix_roughness;

uniform vec3 cam_pos;
uniform int debug_view_mode;
uniform int albedo_interp;
uniform bool ycbcr_interp;

uniform float height_factor;
uniform bool no_warp;
uniform bool wireframe;

uniform float hf_1;
uniform float hf_2;
uniform float textureSize;

uniform float screen_width;
uniform float screen_height;

out vec4 out_color;

float inv_texSize = 1.0f / textureSize;
const float sigma_gauss = 1.0f / 6.0f;

const float gamma = 2.2;
const float inv_gamma = 1.0 / gamma;

const float horizonFade = 1.3;
const int histogram_size = 4096;

const float M_PI = 3.141592653589793;
const float c_MinRoughness = 0.04;

const uint num_lights = 2;

vec3 light_directions[num_lights] = { 
    2.0f * vec3(-0.5f, 0.5f, 0.5f),
    2.0f * vec3(0.5f, 0.5f, 0.5f) };

vec3 light_colors[num_lights] = {
	0.5 * vec3(1.f, 1.f, 1.f),
	0.5 * vec3(1.f, 1.f, 1.f) };

vec3 sphericalHarmonics[9] = {
    vec3( 0.768273532390594,  0.802344262599945,  0.845567941665649), // L00, irradiance, pre-scaled base
    vec3( 0.110741466283798,  0.247339352965355,  0.373646497726440), // L1-1, irradiance, pre-scaled base
    vec3( 0.509003639221191,  0.543381273746490,  0.465391933917999), // L10, irradiance, pre-scaled base
    vec3(-0.392720639705658, -0.425411045551300, -0.383679330348969), // L11, irradiance, pre-scaled base
    vec3(-0.331697195768356, -0.349437117576599, -0.317423462867737), // L2-2, irradiance, pre-scaled base
    vec3( 0.452859580516815,  0.469741851091385,  0.414534509181976), // L2-1, irradiance, pre-scaled base
    vec3( 0.067333340644836,  0.073102675378323,  0.066335253417492), // L20, irradiance, pre-scaled base
    vec3(-0.522640287876129, -0.558134496212006, -0.504732787609100), // L21, irradiance, pre-scaled base
    vec3( 0.047025177627802,  0.053552210330963,  0.052384618669748), // L22, irradiance, pre-scaled base
};

// https://github.com/KhronosGroup/glTF-WebGL-PBR
struct PBRInfo
{
    float NdotL;               // cos angle between normal and light direction
    float NdotV;               // cos angle between normal and view direction
    float NdotH;               // cos angle between normal and half vector
    float LdotH;               // cos angle between light direction and half vector
    float VdotH;               // cos angle between view direction and half vector
    float roughness;           // roughness value, as authored by the model
                               // creator (input to shader)
    float metalness;           // metallic value at the surface
    vec3 reflectance0;         // full reflectance color (normal incidence angle)
    float reflectance90;       // reflectance color at grazing angle
    float alphaRoughness;      // roughness mapped to a more linear change in the
                               // roughness (proposed by [2])
    vec3 diffuseColor;         // color contribution from diffuse lighting
    vec3 specularColor;        // color contribution from specular lighting
};

// #####################
// #### Color Utils ####
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

vec3 rgb2ycbcr(vec3 rgb)
{
	return vec3(
		.299f		* rgb.r + .587f		* rgb.g + .114f		* rgb.b,
		-.168736f	* rgb.r - .331264f	* rgb.g + .5f		* rgb.b + 0.5f,
		.5f			* rgb.r - .418688f	* rgb.g - .081312f	* rgb.b + 0.5f
	);
}

vec3 ycbcr2rgb(vec3 ycbcr)
{
	ycbcr -= vec3(0.f, 0.5f, 0.5f);
	return vec3(
		ycbcr.x					+ 1.402f	* ycbcr.z,
		ycbcr.x - 0.344136f * ycbcr.y - .714136f	* ycbcr.z,
		ycbcr.x					+ 1.772f	* ycbcr.y
	);
}

// #####################
// #### Filament IBL ###
// #####################
//
// https://google.github.io/filament/Filament.html#lighting/imagebasedlights
//

// We can use only the first 2 bands for better performance
vec3 irradianceSH(vec3 n) 
{
return
    sphericalHarmonics[0]
    + sphericalHarmonics[1] * (n.y)
    + sphericalHarmonics[2] * (n.z)
    + sphericalHarmonics[3] * (n.x)
    + sphericalHarmonics[4] * (n.y * n.x)
    + sphericalHarmonics[5] * (n.y * n.z)
    + sphericalHarmonics[6] * (3.0 * n.z * n.z - 1.0)
    + sphericalHarmonics[7] * (n.z * n.x)
    + sphericalHarmonics[8] * (n.x * n.x - n.y * n.y);
}

// NOTE: this is the DFG LUT implementation of the function above
vec2 prefilteredDFG_LUT(float coord, float NoV) 
{
    // coord = sqrt(roughness), which is the mapping used by the
    // IBL prefiltering code when computing the mipmaps
    return textureLod(dfgLut, vec2(NoV, coord), 0.0).rg;
}

vec3 evaluateSpecularIBL(vec3 r, float roughness) 
{
    // This assumes a 256x256 cubemap, with 9 mip levels
    float lod = 8.0 * roughness;
    return textureLod(environmentMap, r, lod).rgb;
}

float computeSpecularAO(float NoV, float ao, float roughness) {
    return clamp(pow(NoV + ao, exp2(-16.0 * roughness - 1.0)) - 1.0 + ao, 0.0, 1.0);
}

vec3 evaluateIBL(PBRInfo pbrInputs, vec3 n, vec3 r) 
{
    // Specular indirect
    vec3 indirectSpecular = evaluateSpecularIBL(r, pbrInputs.roughness);
    vec2 env = prefilteredDFG_LUT(pbrInputs.roughness, pbrInputs.NdotV);
    vec3 specularColor = pbrInputs.reflectance0 * env.x + pbrInputs.reflectance90 * env.y;

    // Diffuse indirect
    // We multiply by the Lambertian BRDF to compute radiance from irradiance
    // With the Disney BRDF we would have to remove the Fresnel term that
    // depends on NoL (it would be rolled into the SH). The Lambertian BRDF
    // can be baked directly in the SH to save a multiplication here
    vec3 indirectDiffuse = max(irradianceSH(n), 0.0) / M_PI;

    float ao = texture(ssaoTexture, vec2(gl_FragCoord.x/screen_width, gl_FragCoord.y/screen_height)).r;

    indirectDiffuse *= ao;
    indirectSpecular *= computeSpecularAO( pbrInputs.NdotV, ao, pbrInputs.roughness);

    // horizon occlusion with falloff, should be computed for direct specular too
    float horizon = min(1.0 + dot(r, n), 1.0);
    indirectSpecular *= horizon * horizon;

    // Indirect contribution
    return pbrInputs.diffuseColor * indirectDiffuse + indirectSpecular * specularColor;
}

float horizonFading(float ndl, float horizonFade)
{
    float horiz = clamp(1.0 + horizonFade * ndl, 0.0, 1.0);
    return horiz * horiz;
}

// #######################
// #### Frostbite PBR ####
// #######################

vec3 F_Schlick(vec3 f0, float f90, float u)
{
    return f0 + (f90 - f0) * pow(1.f - u, 5.f);
}

float Fr_DisneyDiffuse(float NdotV, float NdotL, float LdotH, float linearRoughness)
{
    float energyBias = mix(0., 0.5, linearRoughness);
    float energyFactor = mix(1.0, 1.0 / 1.51, linearRoughness);
    float fd90 = energyBias + 2.0 * LdotH * LdotH * linearRoughness;
    vec3 f0 = vec3(1.0f, 1.0f, 1.0f);
    float lightScatter = F_Schlick(f0, fd90, NdotL).r;
    float viewScatter = F_Schlick(f0, fd90, NdotV).r;

    return lightScatter * viewScatter * energyFactor;
}

float V_SmithGGXCorrelated(float NdotL, float NdotV, float alphaG)
{
    // Optimized version of G_SmithGGX Correlated
    float alphaG2 = alphaG * alphaG;
    // Caution : the " NdotL *" and " NdotV *" are explicitely inversed , this is not a mistake .
    float Lambda_GGXV = NdotL * sqrt((-NdotV * alphaG2 + NdotV) * NdotV + alphaG2);
    float Lambda_GGXL = NdotV * sqrt((-NdotL * alphaG2 + NdotL) * NdotL + alphaG2);

    return 0.5f / (Lambda_GGXV + Lambda_GGXL);
}

float D_GGX(float NdotH, float m)
{
    // Divide by PI is apply later
    float m2 = m * m;
    float f = (NdotH * m2 - NdotH) * NdotH + 1;
    return m2 / (f * f);
}

// #######################
// #### Gaussinization ###
// #######################

float sqr(float x) 
{
    return x*x;
}

// from https://benedikt-bitterli.me/histogram-tiling/
float soft_clipping(float x, float W) 
{
    float u = x > 0.5 ? 1.0 - x : x;
    float result;
    if (u >= 0.5 - 0.25*W)
        result = (u - 0.5)/W + 0.5;
    else if (W >= 2.0/3.0)
        result = 8.0*(1.0/W - 1.0)*sqr(u/(2.0 - W)) + (3.0 - 2.0/W)*u/(2.0 - W);
    else if (u >= 0.5 - 0.75*W)
        result = sqr((u - (0.5 - 0.75*W))/W);
    else
        result = 0.0;
    if (x > 0.5)
        result = 1.0 - result;
    return result;
}

float Erf(float x)
{
	// Save the sign of x
	int sign = 1;
	if (x < 0)
		sign = -1;
	x = abs(x);

	// A&S formula 7.1.26
	float t = 1.0f / (1.0f + 0.3275911f * x);
	float y = 1.0f - (((((1.061405429f * t + -1.453152027f) * t) + 1.421413741f)
		* t + -0.284496736f) * t + 0.254829592f) * t * exp(-x * x);

	return sign * y;
}

float CDFTruncated(float x, float mu, float sigma)
{
	float U = 0.5f * (1 + Erf((x - mu) / (sigma * sqrt(2.0f))) / Erf(1 / (2.0f * sigma * sqrt(2.0f))));
	return U;
}

float interpolation_Burley_Y(float gaussColor_, float gaussColor2_, float mix_val)
{
    float t = mix_val;
    float W = sqrt(pow(1.0f - t, 2.0f) + pow(t, 2.0f));

    float G_mix = mix(gaussColor_, gaussColor2_, t);
    G_mix = soft_clipping(G_mix, W);

    float lut_idx = clamp(G_mix, 0.f, 1.f);

    float res = mix(
        texelFetch(img1_inv_cdf, ivec2((histogram_size - 1) * CDFTruncated(lut_idx, 0.5f, sigma_gauss), 0), 0).r,
        texelFetch(img2_inv_cdf, ivec2((histogram_size - 1) * CDFTruncated(lut_idx, 0.5f, sigma_gauss), 0), 0).r, t);

    return res;
}

vec3 interpolation_Burley(vec3 gaussColor_, vec3 gaussColor2_, float mix_val)
{
    float t = mix_val;
    float W = sqrt(pow(1.0f - t, 2.0f) + pow(t, 2.0f));

    vec3 G_mix = mix(gaussColor_, gaussColor2_, t);
    G_mix = vec3(
        soft_clipping(G_mix.x, W),
        soft_clipping(G_mix.y, W),
        soft_clipping(G_mix.z, W));

    vec3 lut_idx = clamp(G_mix, 0.f, 1.f);

    vec3 res = vec3(
        mix(
            texelFetch(img1_inv_cdf, ivec2((histogram_size - 1) * CDFTruncated(lut_idx.x, 0.5f, sigma_gauss), 0), 0).r,
            texelFetch(img2_inv_cdf, ivec2((histogram_size - 1) * CDFTruncated(lut_idx.x, 0.5f, sigma_gauss), 0), 0).r, t),
        mix(
            texelFetch(img1_inv_cdf, ivec2((histogram_size - 1) * CDFTruncated(lut_idx.y, 0.5f, sigma_gauss), 1), 0).r,
            texelFetch(img2_inv_cdf, ivec2((histogram_size - 1) * CDFTruncated(lut_idx.y, 0.5f, sigma_gauss), 1), 0).r, t),
        mix(
            texelFetch(img1_inv_cdf, ivec2((histogram_size - 1) * CDFTruncated(lut_idx.z, 0.5f, sigma_gauss), 2), 0).r,
            texelFetch(img2_inv_cdf, ivec2((histogram_size - 1) * CDFTruncated(lut_idx.z, 0.5f, sigma_gauss), 2), 0).r, t));

    return res;
}

vec3 getAlbedoInterpolation(float mix_val, vec2 warped_texcoords_1, vec2 warped_texcoords_2)
{
    vec3 baseColor = texture(color_map, warped_texcoords_1).rgb;
    vec3 baseColor2 = texture(color_map_2, warped_texcoords_2).rgb;
    vec3 gaussColor = texture(gaussianized_col1, warped_texcoords_1).rgb;
    vec3 gaussColor2 = texture(gaussianized_col2, warped_texcoords_2).rgb;
    switch (albedo_interp)
	{
    // Linear Albedo Interpolation
	case 1:
        if (ycbcr_interp)
            return ycbcr2rgb(mix(rgb2ycbcr(baseColor), rgb2ycbcr(baseColor2), mix_val));
        else
            return mix(baseColor, baseColor2, mix_val);
    // Burley Interpolation
    case 3:
    {
        if (ycbcr_interp)
            return ycbcr2rgb(vec3(
                    interpolation_Burley_Y(gaussColor.x, gaussColor2.x, mix_val),
                    mix(gaussColor.yz, gaussColor2.yz, mix_val)));
        else
            return interpolation_Burley(gaussColor, gaussColor2, mix_val);
    }
    default:
        break;
    }
    return vec3(1, 0, 0);
}

// #############################
// #### Normal Reorientation ###
// #############################

void get_values_from_height(out float tab[9], sampler2D sampler_, vec2 tex_coord, float w, float h)
{
	tab[0] = texture(sampler_, tex_coord + vec2( -w, -h) ).x;
	tab[1] = texture(sampler_, tex_coord + vec2( 0., -h) ).x;
	tab[2] = texture(sampler_, tex_coord + vec2(  w, -h) ).x;
	tab[3] = texture(sampler_, tex_coord + vec2( -w, 0.) ).x;
	tab[4] = texture(sampler_, tex_coord + vec2( 0., 0.) ).x;
	tab[5] = texture(sampler_, tex_coord + vec2(  w, 0.) ).x;
	tab[6] = texture(sampler_, tex_coord + vec2( -w, h) ).x;
	tab[7] = texture(sampler_, tex_coord + vec2( 0., h) ).x;
	tab[8] = texture(sampler_, tex_coord + vec2(  w, h) ).x;
}

vec3 get_normal_sobel(vec2 tex_coord, int mat_idx, float hf1_, float hf2_)
{
	float w = inv_texSize;
    float h = inv_texSize;

    float n[9];
	if (mat_idx == 1)
		get_values_from_height(n, height_map, 	tex_coord, w, h);
	else if (mat_idx == 2)
		get_values_from_height(n, height_map_2, tex_coord, w, h);

    float sobel_x = n[2] + (2.0f * n[5]) + n[8] - (n[0] + (2.0f * n[3]) + n[6]);
  	float sobel_y = n[0] + (2.0f * n[1]) + n[2] - (n[6] + (2.0f * n[7]) + n[8]);

    if (mat_idx == 1)
		return vec3(-sobel_x, sobel_y, hf1_);
	else if (mat_idx == 2)
		return vec3(-sobel_x, sobel_y, hf2_);
    else
        return vec3(0);
}

float fetch_mix_height(vec2 warpTexCoord1, vec2 warpTexCoord2, vec2 delta, float mix_height_)
{
    if (no_warp)
    {
        return mix(
            texture(height_map,     warpTexCoord1 + delta).r,
            texture(height_map_2,   warpTexCoord1 + delta).r,
            mix_height_);
    }
    else
    {
        return mix(
            texture(height_map,     warpTexCoord1 + delta).r,
            texture(height_map_2,   warpTexCoord2 + delta).r,
            mix_height_);
    }
}
    
vec3 get_geometric_normal()
{
    return normalize(cross(dFdx(worldPos), dFdy(worldPos)));
}

vec3 addDetailRNM(vec3 baseNormal, vec3 detailNormal)
{
    vec3 t = baseNormal + vec3(0, 0, 1);
    vec3 u = detailNormal * vec3(-1, -1, 1);
    return normalize(t * dot(t, u) / t.z - u);
}

vec3 getDetailRNM(vec3 baseNormal, vec3 detailNormal)
{
    vec3 t = baseNormal + vec3(0, 0, 1);
    vec3 u = detailNormal * vec3(-1, -1, 1);
    t.z *= -1.0f;
    return normalize(t * dot(t, u) / (-t.z) - u);
}

vec3 Slerp(vec3 start, vec3 end, float percent) 
{
    float angle = acos(clamp(dot(start, end), -0.99999f, 0.99999f));
    float sinTotal = sin(angle);

    float ratioA = sin((1 - percent) * angle) / sinTotal;
    float ratioB = sin(percent * angle) / sinTotal;
    vec3 out_vec = ratioA * start + ratioB * end;

    return normalize(out_vec);
}

// #define use_RNM

vec3 getNormal(vec2 warpTexCoord1, vec2 warpTexCoord2, float mix_normal_)
{
    vec3 n1 = 2 * texture(normal_map,   warpTexCoord1).rgb - 1;
    vec3 n2 = 2 * texture(normal_map_2, warpTexCoord2).rgb - 1;
    vec3 n = Slerp(n1, n2, mix_normal_);

// #ifdef use_RNM

    if(!no_warp)
    {
        // get normals from heights
        vec3 nFromH1 = normalize(get_normal_sobel(warpTexCoord1, 1, hf_1, hf_2));
        vec3 nFromH2 = normalize(get_normal_sobel(warpTexCoord2, 2, hf_1, hf_2));

        // get details maps from original normals and n_fH
        vec3 detailNormal1 = getDetailRNM(nFromH1, n1);
        vec3 detailNormal2 = getDetailRNM(nFromH2, n2);

        // interpolate details
        vec3 detail_normal = Slerp(detailNormal1, detailNormal2, mix_normal_);
        
        // interpolate height and get normals
        vec3 nFromHeight_mix = get_geometric_normal();
        
        // add details
        n = addDetailRNM(nFromHeight_mix, detail_normal);
    }

// #endif

    else
    {
        vec3 ng = normal;
        vec3 t = tangent.xyz;
        vec3 b = cross(ng, t);

        n = mat3(t, b, ng) * n;
    }

    return n;
}

vec3 debug_view_color(
    vec3 input_col,
    vec3 baseColor_,
    vec3 normal_,
    vec3 view_,
    float height_,
    float metallic_,
    float roughness_,
    float mix_normal_,
    vec2 warped_texcoords_1_,
    vec2 warped_texcoords_2_)
{
    vec3 color = input_col;
    switch (debug_view_mode)
	{
    //default PBR
	case 0:
		break;
    //Base Color
	case 1:
        color = baseColor_;
		break;
    //Height
	case 2:
		color = vec3(height_ + 0.5);
		break;
    //Metallic
	case 3:
        color = vec3(metallic_);
		break;
    //Normal [0, 1]
	case 4:
        color = vec3((normal_ + 1) / 2);
		break;
    //Roughness
	case 5:
        color = vec3(roughness_);
		break;
    //Detail normal
	case 6:
    {
        vec3 n1 = 2 * texture(normal_map,   warped_texcoords_1_).rgb - 1;
        vec3 n2 = 2 * texture(normal_map_2, warped_texcoords_2_).rgb - 1;

        // get normals from heights
        vec3 nFromH1 = normalize(get_normal_sobel(warped_texcoords_1_, 1, hf_1, hf_2));
        vec3 nFromH2 = normalize(get_normal_sobel(warped_texcoords_2_, 2, hf_1, hf_2));

        // get details maps from original normals and n_fH
        vec3 detailNormal1 = getDetailRNM(nFromH1, n1);
        vec3 detailNormal2 = getDetailRNM(nFromH2, n2);

        // interpolate details
        color = Slerp(detailNormal1, detailNormal2, mix_normal_) * 0.5 + 0.5;
        break;
    }
    //Geometric normal
	case 7:
    {
        color = get_geometric_normal() * 0.5 + 0.5;
		break;
    }
    // SSAO
	case 8:
        color = vec3(texture(ssaoTexture, vec2(gl_FragCoord.x/screen_width, gl_FragCoord.y/screen_height)).r);
		break;
    default:
        break;
	}

    return color;
}

void main()
{
    float new_mix_color = mix_color;
    float new_mix_height = mix_height;
    float new_mix_normal = mix_normal;
    float new_mix_metallic = mix_metallic;
    float new_mix_roughness = mix_roughness;
    vec2 new_tex_coord = texcoord;

    // #######################
    // #### Interpolation ####
    // #######################

    vec2 warped_texcoords_1 = new_tex_coord;
    vec2 warped_texcoords_2 = texture(warpTexture, new_tex_coord).rg;

    if (no_warp)
    {
        warped_texcoords_1 = new_tex_coord;
        warped_texcoords_2 = new_tex_coord;
    }

    // ################
    // #### Albedo ####
    // ################

    // The albedo may be defined from a base texture or a flat color
    vec3 baseColor = getAlbedoInterpolation(new_mix_color, warped_texcoords_1, warped_texcoords_2);

    // ############################
    // #### Metallic Roughness ####
    // ############################

    float mSample = texture(metallic_map, warped_texcoords_1).r;
    float rSample = texture(roughness_map, warped_texcoords_1).r;
    float mSample2 = texture(metallic_map_2, warped_texcoords_2).r;
    float rSample2 = texture(roughness_map_2, warped_texcoords_2).r;
    mSample = mix(mSample, mSample2, new_mix_metallic);
    rSample = mix(rSample, rSample2, new_mix_roughness);
    float metallic = clamp(mSample, 0.0, 1.0);
    float roughness = clamp(rSample, c_MinRoughness, 1.0);
    float alphaRoughness = roughness * roughness;

    // ################
    // #### Normal ####
    // ################

    vec3 n = getNormal(warped_texcoords_1, warped_texcoords_2, new_mix_normal);

    // #######################
    // #### PBR Rendering ####
    // #######################

    vec3 f0 = vec3(0.04);
    vec3 diffuseColor = baseColor.rgb * (vec3(1.0) - f0);
    diffuseColor *= 1.0 - metallic;
    vec3 specularColor = mix(f0, baseColor.rgb, metallic);

    // Compute reflectance.
    float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);

    // For typical incident reflectance range (between 4% to 100%) set the grazing
    // reflectance to 100% for typical fresnel effect.
    // For very low reflectance range on highly diffuse objects (below 4%),
    // incrementally reduce grazing reflecance to 0%.
    float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);

    vec3 specularEnvironmentR0 = specularColor.rgb;
    // Anything less than 2% is physically impossible and is instead considered to be shadowing. Compare to "Real-Time-Rendering" 4th editon on page 325.
    float specularEnvironmentR90 = reflectance90;

    vec3 v = normalize(cam_pos - worldPos); // Vector from surface point to camera

    vec3 Fr_total = vec3(0);
    vec3 Fd_total = vec3(0);
    vec3 color = vec3(0);

    // #######################
    // #### Frostbite PBR ####
    // #######################

    for (uint i = 0; i < num_lights; i++)
    {
        vec3 l = normalize(light_directions[i] - worldPos); // Vector from surface point to light
        vec3 h = normalize(l + v);                          // Half vector between both l and v
        
        float distance = length(light_directions[i] - worldPos);
        float attenuation = 1.0 / (distance * distance);
        vec3 radiance = light_colors[i].rgb * attenuation;

        float NdotL = clamp(dot(n, l), 0.001, 1.0);
        float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);
        float NdotH = clamp(dot(n, h), 0.0, 1.0);
        float LdotH = clamp(dot(l, h), 0.0, 1.0);
        float VdotH = clamp(dot(v, h), 0.0, 1.0);

        // Specular BRDF
        vec3 F = F_Schlick(specularEnvironmentR0 , specularEnvironmentR90 , LdotH);
        float Vis = V_SmithGGXCorrelated ( NdotV , NdotL , roughness );
        float D = D_GGX ( NdotH , roughness );
        vec3 Fr = D * F * Vis / M_PI ;

        // Diffuse BRDF
        vec3 Fd = diffuseColor * Fr_DisneyDiffuse ( NdotV , NdotL , LdotH , alphaRoughness ) / M_PI ;

        // Calculate the shading terms for the microfacet specular shading model
		Fr_total += Fr;
		Fd_total += Fd;

        // Obtain final intensity as reflectance (BRDF) scaled by the energy of the light (cosine law)
        color += NdotL * radiance * (Fd + Fr);
    }

    // #######################
    // #### IBL ##############
    // #######################

    vec3 l = vec3(0); // Vector from surface point to light
    vec3 h = normalize(l + v); // Half vector between both l and v
    vec3 reflection = normalize(reflect(-v, n));

    float NdotL = clamp(dot(n, l), 0.001, 1.0);
    float NdotV = clamp(abs(dot(n, v)), 0.0001, 1.0);
    float NdotH = clamp(dot(n, h), 0.0, 1.0);
    float LdotH = clamp(dot(l, h), 0.0, 1.0);
    float VdotH = clamp(dot(v, h), 0.0, 1.0);

    PBRInfo pbrInputs = PBRInfo(NdotL, NdotV, NdotH, LdotH, VdotH, roughness, metallic,
                specularEnvironmentR0, specularEnvironmentR90, alphaRoughness,
                diffuseColor, specularColor);

    color += evaluateIBL(pbrInputs, n, reflection); 

    // #######################
    // #### Visualization ####
    // #######################

    float height = texture(height_map, warped_texcoords_1).r - 0.5;
    float height2 = texture(height_map_2, warped_texcoords_2).r - 0.5;
    height = mix(height, height2, new_mix_height);

    color = debug_view_color(
        color,
        baseColor,
        n,
        v,
        height,
        metallic,
        roughness,
        new_mix_normal,
        warped_texcoords_1,
        warped_texcoords_2);

    out_color = vec4(color, 0.0);

    if (wireframe && debug_view_mode == 8)
        out_color = vec4(1.0);
}