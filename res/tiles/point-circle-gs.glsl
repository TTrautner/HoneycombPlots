#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;


uniform mat4 modelViewProjectionMatrix;

uniform float pointCircleRadius;
uniform float kdeRadius;
uniform float aspectRatio;

//imposter space coordinates
out vec2 isCoords;

/*
This shader is used to render both
circles for the point circle scatterplot
Kernel Density Estimation

Since they can have different radius, we create imposters which are the size of the bigger one of the 2
In the fragment shader, we discard the fragments accordingly
*/
void main()
{

	float radius = max(kdeRadius, pointCircleRadius);

    // create bounding box geometry
	vec4 pos = modelViewProjectionMatrix * gl_in[0].gl_Position + vec4(radius, radius/aspectRatio,0.0,0.0);
    isCoords = vec2(1, 1);
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * gl_in[0].gl_Position + vec4(-radius,radius/aspectRatio,0.0,0.0);
    isCoords = vec2(-1, 1);
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * gl_in[0].gl_Position + vec4(radius, -radius/aspectRatio,0.0,0.0);
    isCoords = vec2(1, -1);
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * gl_in[0].gl_Position + vec4(-radius, -radius/aspectRatio,0.0,0.0);
	isCoords = vec2(-1, -1);
    gl_Position = pos;
	EmitVertex();

	EndPrimitive();
}