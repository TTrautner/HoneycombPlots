#version 450

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

out vec4 gFragmentPosition;

void main()
{
	vec4 pos = gl_in[0].gl_Position + vec4(1.0,1.0,0.0,0.0);
	gFragmentPosition = pos;
	gl_Position = pos;
	EmitVertex();

	pos = gl_in[0].gl_Position + vec4(-1.0,1.0,0.0,0.0);
	gFragmentPosition = pos;
	gl_Position = pos;
	EmitVertex();

	pos = gl_in[0].gl_Position + vec4(1.0,-1.0,0.0,0.0);
	gFragmentPosition = pos;
	gl_Position = pos;
	EmitVertex();

	pos = gl_in[0].gl_Position + vec4(-1.0,-1.0,0.0,0.0);
	gFragmentPosition = pos;
	gl_Position = pos;
	EmitVertex();

	EndPrimitive();
}