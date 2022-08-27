#version 400

layout(triangles) in;
layout(triangle_strip, max_vertices = 3) out;
in vec4 tePatchDistance[3];
out vec4 gPatchDistance;

uniform mat4 modelView;

void main()
{   
    gPatchDistance = tePatchDistance[0];
    gl_Position = gl_in[0].gl_Position;
	EmitVertex();

    gPatchDistance = tePatchDistance[1];
    gl_Position = gl_in[1].gl_Position;
	EmitVertex();

    gPatchDistance = tePatchDistance[2];
    gl_Position = gl_in[2].gl_Position;
	EmitVertex();

    EndPrimitive();
}