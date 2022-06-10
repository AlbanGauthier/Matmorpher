#version 450 core

layout(triangles, equal_spacing, ccw) in;

layout(location = 0) in vec3 position[];
layout(location = 1) in vec2 texcoord[];
layout(location = 2) in vec3 normal[];
layout(location = 3) in vec4 tangent[];

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec2 out_texcoord;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec4 out_tangent;

uniform mat4 mat_proj;
uniform mat4 mat_view;

uniform float interpolation_value;
uniform int rendered_mesh;

uniform float mix_height;
uniform float height_factor;
uniform bool no_warp;

layout(binding = 1) uniform sampler2D height_map;
layout(binding = 6) uniform sampler2D height_map_2;

layout(binding = 10) uniform sampler2D warpTexture;

#define M_PI 3.1415926538

const float resolution = 128.0f;
const float stepPhi =   2.f * M_PI;
const float stepTheta = 1.f * M_PI;

vec3 polar2Cartesian(
  const float phi, const float theta) 
{
  return vec3(
    sin(theta) * sin(phi),
    cos(theta),
    sin(theta) * cos(phi));
}

//TODO: watch alignment
struct Vertex
{
    vec3 position;
    vec2 texcoord;
    vec3 normal;
    vec4 tangent;
    float disp;
};

Vertex getVertex(vec3 bcoord, int ilod)
{
    Vertex vert;
    vert.position = bcoord[0] * position[0] 
                + bcoord[1] * position[1] 
                + bcoord[2] * position[2];

    vert.texcoord = bcoord[0] * texcoord[0] 
                + bcoord[1] * texcoord[1] 
                + bcoord[2] * texcoord[2];

    vert.normal = bcoord[0] * normal[0] 
                + bcoord[1] * normal[1] 
                + bcoord[2] * normal[2];

    vert.tangent = bcoord[0] * tangent[0] 
                + bcoord[1] * tangent[1] 
                + bcoord[2] * tangent[2];

    return vert;
}

void main()
{
    int lod_level = 0;

    Vertex vert = getVertex(gl_TessCoord, lod_level);

    switch(rendered_mesh)
    {
    case 0: // Plane
    {
        if (!no_warp)
        {
            float interp_val = interpolation_value;

            vec2 warped = texture(warpTexture, vert.texcoord.xy).rg;
            vec2 mix_coord = mix(vert.texcoord.xy, warped, interp_val);

            vert.texcoord = vert.texcoord.xy;
            vert.position = vec3(2 * mix_coord - 1, vert.position.z);
        }
        break;
    }
    case 1: // Sphere
    {
        vec2 mod_texcoords = vec2(mod(vert.texcoord.x, 1), mod(vert.texcoord.y, 1));

        if (!no_warp)
        {
            vert.texcoord = vert.texcoord - mod_texcoords;

            vec2 warped = texture(warpTexture, mod_texcoords).rg;
            vec2 mix_coord = mix(mod_texcoords, warped, interpolation_value);

            mix_coord = mix_coord + vert.texcoord;
            
            mix_coord = vec2( 0.25 * (mix_coord.x - 0.5), -0.5 * (mix_coord.y - 0.5) );

            float phi   = mix_coord.x * stepPhi + M_PI;
            float theta = mix_coord.y * stepTheta;
            vec3 warped_pos = polar2Cartesian(phi, theta);
            vert.position = warped_pos;
        }
        else
        {
            vert.position = normalize(vert.position);
        }

        vert.texcoord = mod_texcoords;

        break;
    }
    }

    float new_mix_height = mix_height;

    vec2 warped_texcoords = texture(warpTexture, vert.texcoord).rg;

    if (no_warp) 
    {
        warped_texcoords = vert.texcoord;
    }

    float height = mix(
              texture(height_map, vert.texcoord).x,
              texture(height_map_2, warped_texcoords).x, mix_height);

    vert.disp = height - 0.5; // [0,1] -> [-1,1]

    switch(rendered_mesh)
    {
    case 0:
        vert.position += vert.disp * height_factor * vec3(0, 0, 1);
        break;
    case 1:
    case 2:
        vert.position += vert.disp * height_factor * vert.position;
        break;
    }    

    gl_Position = mat_proj * mat_view * vec4(vert.position, 1.0);

    out_position = vert.position;
    out_texcoord = vert.texcoord;
    out_normal   = vert.normal;
    out_tangent  = vert.tangent;
}