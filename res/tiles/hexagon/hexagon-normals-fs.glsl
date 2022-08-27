#version 450
#include "/defines.glsl"
#include "/globals.glsl"
#include "/hex/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

// we safe each value of the normal (vec4) seperately + accumulated kde height = 5 values
// since we deal with floats and SSBOs can only perform atomic operations on int or uint
// we mulitply all value with bufferAccumulationFactor and then cast them to int when accumulating
// and the divide them by bufferAccumulationFactor when reading themlayout(std430, binding = 0) buffer tileNormalsBuffer
layout(std430, binding = 0) buffer tileNormalsBuffer
{
    int tileNormals[];
};

in vec4 boundsScreenSpace;
in vec2 rectSizeScreenSpace;
in float tileSizeScreenSpace;

uniform sampler2D accumulateTexture;
uniform sampler2D kdeTexture;

//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

uniform int max_rect_col;
uniform int max_rect_row;

uniform float bufferAccumulationFactor;

//--out
layout(location = 0) out vec4 hexTilesTexture;

void main()
{
    vec2 minBounds = vec2(boundsScreenSpace[2], boundsScreenSpace[3]);
    vec2 maxBounds = vec2(boundsScreenSpace[0], boundsScreenSpace[1]);

    //some pixels of rect row 0 do not fall inside a hexagon.
    //there we need to discard them
    if(discardOutsideOfGridFragments(vec2(gl_FragCoord), minBounds, maxBounds, rectSizeScreenSpace, max_rect_col, max_rect_row)){
        discard;
    }

    // get hexagon coordinates of fragment
    vec2 hex = matchPointWithHexagon(vec2(gl_FragCoord), max_rect_col, max_rect_row, rectSizeScreenSpace.x, rectSizeScreenSpace.y,
                                     minBounds, maxBounds);

    // we don't want to render fragments outside the grid
    if(hex.x < 0 || hex.y < 0){
        discard;
    }
    // get value from accumulate texture
    float hexValue = texelFetch(accumulateTexture, ivec2(hex.x, hex.y), 0).r;
    
    // we don't want to render empty hexs
    if (hexValue < 0.00000001)
    {
        discard;
    }
    
   // get value from density normals texture
    vec4 fragmentNormal = vec4(calculateNormalFromHeightMap(ivec2(gl_FragCoord.xy), kdeTexture), 1.0f);
    fragmentNormal *= bufferAccumulationFactor;
   
    // accumulate fragment normals
    for(int i = 0; i < 4; i++){
        int intValue = int(fragmentNormal[i]);

        atomicAdd(tileNormals[int((hex.x*(maxTexCoordY+1) + hex.y) * 5 + i)], intValue);
    }

    //accumulate height of kdeTexture
    float kdeHeight = texelFetch(kdeTexture, ivec2(gl_FragCoord.xy), 0).r;
    kdeHeight *= bufferAccumulationFactor;
    atomicAdd(tileNormals[int((hex.x*(maxTexCoordY+1) + hex.y) * 5 + 4)], int(kdeHeight));
}
