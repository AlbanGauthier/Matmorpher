#version 450 core

layout(location = 0) in vec3 in_position;
layout(location = 1) in vec2 in_texcoord;
layout(location = 2) in vec3 in_normal;
layout(location = 3) in vec4 in_tangent;

layout(location = 0) out vec3 out_position;
layout(location = 1) out vec2 out_texcoord;
layout(location = 2) out vec3 out_normal;
layout(location = 3) out vec4 out_tangent;

layout(binding = 10) uniform sampler2D warpTexture;

uniform int rendered_mesh;

uniform mat4 mat_proj;
uniform mat4 mat_view;

void main() {

  out_normal  = in_normal;  //from Mesh
  out_tangent = in_tangent; //from Mesh

  switch(rendered_mesh)
  {
    case 0: // Plane
    {
      out_position  = vec3(2 * in_position.xy - 1.0, in_position.z);  // in [-1.0,1.0]
      out_texcoord  = in_position.xy;                                 // in [0.0, 1.0]
      break;
    }
    case 1: // Sphere
    {
      out_position = in_position;
      out_texcoord = vec2(4 * in_texcoord.x + 0.5, -2 * in_texcoord.y + 0.5);
      break;
    }
  }
}