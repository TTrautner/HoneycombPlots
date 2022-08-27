#version 450
#include "/defines.glsl"
#include "/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(std430, binding = 0) buffer valueMaxBuffer
{
	uint maxAccumulate;
    uint maxPointAlpha;
};

#ifdef RENDER_TILES
    in vec4 boundsScreenSpace;

    uniform sampler2D accumulateTexture;

    //min = 0
    uniform int maxTexCoordX;
    uniform int maxTexCoordY;
#endif

uniform sampler2D pointCircleTexture;

void main()
{
    
    #ifdef RENDER_TILES

        // to get intervals from 0 to maxTexCoord, we map the original Point interval to maxTexCoord+1
        // If the current index = maxIndex+1, we take the maxTexCoord instead
        int tileX = min(maxTexCoordX, mapInterval(gl_FragCoord.x, boundsScreenSpace[2], boundsScreenSpace[0], maxTexCoordX+1));
        int tileY = min(maxTexCoordY, mapInterval(gl_FragCoord.y, boundsScreenSpace[3], boundsScreenSpace[1], maxTexCoordY+1));

        // get value from accumulate texture
        float tileAccValue = texelFetch(accumulateTexture, ivec2(tileX,tileY), 0).r;

        // floatBitsToUint: https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/floatBitsToInt.xhtml
        uint uinttileAccValue = floatBitsToUint(tileAccValue);

        // identify max value
        atomicMax(maxAccumulate, uinttileAccValue);
    #endif

    // point circle max alpha
    float pointAlpha = texelFetch(pointCircleTexture, ivec2(gl_FragCoord.x, gl_FragCoord.y),0).a;
	uint uintPointAlphaValue = floatBitsToUint(pointAlpha);

	atomicMax(maxPointAlpha, uintPointAlphaValue);
}
