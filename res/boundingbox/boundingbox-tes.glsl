#version 400

layout(quads) in;
in vec3 tcPosition[];
out vec4 tePatchDistance;

uniform mat4 projection;
uniform mat4 modelView;

void main()
{
    float u = gl_TessCoord.x;
	float v = gl_TessCoord.y;

    vec3 a = mix(tcPosition[0], tcPosition[1], u);
    vec3 b = mix(tcPosition[3], tcPosition[2], u);

    tePatchDistance = vec4(u, v, 1-u, 1-v);

    vec3 position = mix(a, b, v);
    gl_Position = projection * modelView * vec4(position, 1);
}