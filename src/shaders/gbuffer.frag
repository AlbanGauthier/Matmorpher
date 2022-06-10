#version 450 core

layout(location = 0) in vec3 frag_position;
layout(location = 1) in vec2 frag_texcoord;
layout(location = 2) in vec3 frag_normal;
layout(location = 3) in vec4 frag_tangent;

layout (location = 0) out vec3 gNormal;
layout (location = 1) out vec3 gPosition;

uniform mat4 mat_proj;
uniform mat4 mat_view;

void main()
{
    gNormal = normalize(cross(dFdx(frag_position), dFdy(frag_position)));

    vec4 WorldPos = vec4(frag_position, 1.0f);

    gPosition = (mat_view * WorldPos).xyz;
}