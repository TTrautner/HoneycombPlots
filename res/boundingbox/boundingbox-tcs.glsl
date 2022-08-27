#version 400

layout(vertices = 4) out;
in vec3 vPosition[];
out vec3 tcPosition[];

void main()
{
    tcPosition[gl_InvocationID] = vPosition[gl_InvocationID];

    if (gl_InvocationID == 0)
	{
        gl_TessLevelInner[0] = 1.0;
        gl_TessLevelInner[1] = 1.0;
        gl_TessLevelOuter[0] = 1.0;
        gl_TessLevelOuter[1] = 1.0;
        gl_TessLevelOuter[2] = 1.0;
        gl_TessLevelOuter[3] = 1.0;
    }
}