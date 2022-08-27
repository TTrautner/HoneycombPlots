#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/globals.glsl"

layout(location = 1) in float discrepancy;

uniform int numCols;
uniform int numRows;
uniform float discrepancyDiv;

out vec4 tileDiscrepancy;

void main()
{
	//calculate position of tile center
	float col = mod(gl_VertexID,numCols);
    float row = gl_VertexID/numCols;

    // we only set the red channel, because we only use the color for additive blending
    tileDiscrepancy = vec4(1-discrepancy/discrepancyDiv,0.0f,0.0f,1.0f);

    // NDC - map tile coordinates to [-1,1] (first [0,2] than -1)
	vec2 tileNDC = vec2(((col * 2) / float(numCols)) - 1, ((row * 2) / float(numRows)) - 1);

    gl_Position = vec4(tileNDC, 0.0f, 1.0f);
}