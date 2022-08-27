#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;
layout(location = 1) in float yValue;

void main()
{

	gl_Position = vec4(xValue, yValue, 0.0f, 1.0f);

}