#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//out
layout (location = 0) out vec4 accumulateTexture;

in vec4 vertexColor;

void main(){

	accumulateTexture = vertexColor;
}