#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"
#include "/hex/globals.glsl"

layout(location = 0) in float xValue;
layout(location = 1) in float yValue;

//[x,y]
uniform vec2 maxBounds_rect;
uniform vec2 minBounds_Offset;
//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

uniform int max_rect_col;
uniform int max_rect_row;

uniform float rectHeight;
uniform float rectWidth;

out vec4 vertexColor;

void main()
{

    // get coordinates of point in the hexagon grid
    vec2 hex = matchPointWithHexagon(vec2 (xValue, yValue), max_rect_col, max_rect_row, rectWidth, rectHeight, minBounds_Offset, maxBounds_rect);

    // we only set the red channel, because we only use the color for additive blending
    vertexColor = vec4(1.0f,0.0f,0.0f,1.0f);
    
    // NDC - map hex coordinates to [-1,1] (first [0,2] than -1)
    vec2 hexNDC = vec2(((hex.x * 2) / float(maxTexCoordX + 1)) - 1, ((hex.y * 2) / float(maxTexCoordY + 1)) - 1);

    // we need to map the tile coordinates to NDC so when OpenGL performs the viewpoer transformation
    // we end up with the original tile coordinates again
    gl_Position = vec4(hexNDC, 0.0f, 1.0f);
}