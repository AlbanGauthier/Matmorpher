#version 450 core

// define the number of CPs in the output patch
layout(vertices = 3) out;

/*User input variables*/
layout(location = 0) in vec3 position[];
layout(location = 1) in vec2 texcoord[];
layout(location = 2) in vec3 normal[];
layout(location = 3) in vec4 tangent[];

/*User output variables*/
layout(location = 0) out vec3 out_position[];
layout(location = 1) out vec2 out_texcoord[];
layout(location = 2) out vec3 out_normal[];
layout(location = 3) out vec4 out_tangent[];

uniform int tess_level;

void main()
{
    out_position[gl_InvocationID] = position[gl_InvocationID];
    out_texcoord[gl_InvocationID] = texcoord[gl_InvocationID];
    out_normal[gl_InvocationID]   = normal[gl_InvocationID];
    out_tangent[gl_InvocationID]  = tangent[gl_InvocationID];

    if (gl_InvocationID == 0)
    {
        gl_TessLevelOuter[0] 
            = gl_TessLevelOuter[1] 
            = gl_TessLevelOuter[2] 
            = gl_TessLevelInner[0] 
            = gl_TessLevelInner[1] 
            = tess_level;
    }
}