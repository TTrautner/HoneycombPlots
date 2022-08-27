#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/globals.glsl"

layout(location = 0) in float xValue;

uniform int numCols;
uniform int numRows;

void main()
{
	//calculate position of square center
	float col = mod(gl_VertexID,numCols);
    float row = gl_VertexID/numCols;
	
	gl_Position =  vec4(col, row, 0.0f, 1.0f);
}