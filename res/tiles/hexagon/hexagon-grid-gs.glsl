#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 10) out;

uniform sampler2D accumulateTexture;

uniform int windowHeight;
uniform int windowWidth;

uniform float tileSize;
uniform float gridWidth;
uniform mat4 modelViewProjectionMatrix;

const float PI = 3.1415926;

in VS_OUT {
    vec2 accTexPosition;
} gs_in[];

//in screen space
out float tileSizeSS;
out vec2 tileCenterSS;
out float neighbourValues[6];
out vec2 accCoords;

// i = 0 => right Center
// continue agains clock circle (i = 1 = right Top)
void emitHexagonVertex(int i){
	//flat-topped
	float angle_deg = 60 * i;
	float angle_rad = PI / 180 * angle_deg;

	// Offset from center of point + grid width
	vec4 offset = vec4((tileSize+gridWidth/2.0f) * cos(angle_rad), (tileSize+gridWidth/2.0f) * sin(angle_rad), 0.0, 0.0);
	gl_Position = modelViewProjectionMatrix * (gl_in[0].gl_Position + offset);

	EmitVertex();
}

void emitHexagonCenterVertex(){
	gl_Position = modelViewProjectionMatrix * gl_in[0].gl_Position;
	EmitVertex();
}

void main()
{

    ivec2 tilePosInAccTexture = ivec2(gs_in[0].accTexPosition);
	accCoords = tilePosInAccTexture;

	// get value from accumulate texture
    float hexValue = texelFetch(accumulateTexture, tilePosInAccTexture, 0).r;

	//left top, left bottom, bottom, right bottom, right top, top
	if(mod(tilePosInAccTexture.x, 2) == 0){
		neighbourValues[0] = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x-1, tilePosInAccTexture.y+1), 0).r;
		neighbourValues[1] = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x-1, tilePosInAccTexture.y), 0).r;
		neighbourValues[3] = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x+1, tilePosInAccTexture.y), 0).r;
		neighbourValues[4] = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x+1, tilePosInAccTexture.y+1), 0).r;
	}
	else{
		neighbourValues[0] = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x-1, tilePosInAccTexture.y), 0).r;
		neighbourValues[1] = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x-1, tilePosInAccTexture.y-1), 0).r;
		neighbourValues[3] = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x+1, tilePosInAccTexture.y-1), 0).r;
		neighbourValues[4] = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x+1, tilePosInAccTexture.y), 0).r;
	}
	//top and bottom are independent of column index
	neighbourValues[2] = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x, tilePosInAccTexture.y-1), 0).r;
	neighbourValues[5] = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x, tilePosInAccTexture.y+1), 0).r;

    // we dont want to render the grid for empty hexs
    if(hexValue > 0){

		// tile size in screen space
		tileSizeSS = getScreenSpaceSize(modelViewProjectionMatrix, vec2(tileSize, 0.0f), windowWidth, windowHeight).x;
		// position of tile center in screen space
        tileCenterSS = getScreenSpacePosOfPoint(modelViewProjectionMatrix, vec2(gl_in[0].gl_Position), windowWidth, windowHeight);

		//Emit 10 vertices for triangulated hexagon
		emitHexagonVertex(0);
		emitHexagonCenterVertex();
		emitHexagonVertex(1);
		emitHexagonVertex(2);
		emitHexagonCenterVertex();
		emitHexagonVertex(3);
		emitHexagonVertex(4);
		emitHexagonCenterVertex();
		emitHexagonVertex(5);
		emitHexagonVertex(6);

		EndPrimitive();
	}
}