#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//out
layout (location = 0) out vec4 tileDiscrepancyTexture;

in vec4 tileDiscrepancy;

void main(){

	tileDiscrepancyTexture = tileDiscrepancy;
}