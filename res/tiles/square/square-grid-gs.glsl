#version 450
#include "/defines.glsl"
#include "/globals.glsl"

#define N 4

layout(points) in;
layout(triangle_strip, max_vertices = N+1) out;

uniform sampler2D accumulateTexture;

//[x,y]
uniform vec2 maxBounds_Off;
uniform vec2 minBounds_Off;

uniform int windowHeight;
uniform int windowWidth;

uniform float tileSize;
uniform float gridWidth;
uniform mat4 modelViewProjectionMatrix;

uniform int numCols;
uniform int numRows;

const float PI = 3.1415926;

out float tileSizeSS;
out vec2 tileCenterSS;
out vec4 neighbourValues;

void main()
{
    
    ivec2 tilePosInAccTexture = ivec2(gl_in[0].gl_Position);

    // get value from accumulate texture
    float squareValue = texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x, tilePosInAccTexture.y), 0).r;

    //left,bottom,right,top
    neighbourValues = vec4(texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x-1, tilePosInAccTexture.y), 0).r,
    texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x, tilePosInAccTexture.y-1), 0).r,
    texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x+1, tilePosInAccTexture.y), 0).r,
    texelFetch(accumulateTexture, ivec2(tilePosInAccTexture.x, tilePosInAccTexture.y+1), 0).r);

    // we dont want to render the grid for empty squares
    if(squareValue > 0){

        // map from square space to bounding box space
        float bbX = mapInterval_O(gl_in[0].gl_Position.x, 0, numCols, minBounds_Off[0], maxBounds_Off[0]);
        float bbY = mapInterval_O(gl_in[0].gl_Position.y, 0, numRows, minBounds_Off[1], maxBounds_Off[1]);

        vec4 bbPos = vec4(bbX, bbY, 0.0f, 1.0f);

        // tile site in screen space
        tileSizeSS = getScreenSpaceSize(modelViewProjectionMatrix, vec2(tileSize, 0.0f), windowWidth, windowHeight).x;
        // position of the tile center in screen space
        tileCenterSS = getScreenSpacePosOfPoint(modelViewProjectionMatrix, vec2(bbPos) + tileSize/2.0f, windowWidth, windowHeight);

        //Emit 4 vertices for square
        // square is by gridWidth larger than the original tileSize
        vec4 pos;

        //bottom left
        pos = modelViewProjectionMatrix * (bbPos + vec4(-gridWidth, -gridWidth, 0 ,0));
        gl_Position = pos;
        EmitVertex();
        //bottom right
        pos = modelViewProjectionMatrix * (bbPos + vec4(tileSize+gridWidth, -gridWidth, 0, 0));
        gl_Position = pos;
        EmitVertex();
        //top left
        pos = modelViewProjectionMatrix * (bbPos + vec4(-gridWidth, tileSize+gridWidth, 0, 0));
        gl_Position = pos;
        EmitVertex();
        //top right
        pos = modelViewProjectionMatrix * (bbPos + vec4(tileSize+gridWidth, tileSize+gridWidth, 0, 0));
        gl_Position = pos;
        EmitVertex();

        EndPrimitive();
    }
}