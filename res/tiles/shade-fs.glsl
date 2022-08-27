#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;
layout (location = 0) out vec4 colorTexture;

layout(std430, binding = 6) buffer valueMaxBuffer
{
    uint maxAccumulate;
    uint maxPointAlpha;
};

uniform vec3 backgroundColor;

uniform sampler2D pointChartTexture;
uniform sampler2D pointCircleTexture;
uniform sampler2D tilesTexture;
uniform sampler2D gridTexture;
uniform sampler2D accumulateTexture;
uniform sampler2D kdeTexture;
uniform sampler2D normalsAndDepthTexture;

void main()
{
    
    vec4 col = texelFetch(pointChartTexture, ivec2(gl_FragCoord.xy), 0).rgba;
    float alpha;

    vec4 tilesCol;
    vec4 pointCircleCol;

    // first set color of point circle scatterplot
    #ifdef RENDER_POINT_CIRCLES

        // read from SSBO and convert back to float: --------------------------------------------------------------------
        // - https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/intBitsToFloat.xhtml
        float floatMaxPointAlpha = uintBitsToFloat(maxPointAlpha);

        pointCircleCol = texelFetch(pointCircleTexture, ivec2(gl_FragCoord.xy), 0).rgba;
        //normaliize color with maximum alpha
        pointCircleCol /= floatMaxPointAlpha;

        col = pointCircleCol;
    #endif 

    // get colour resulted form tiling
    #ifdef RENDER_TILES
        tilesCol = texelFetch(tilesTexture, ivec2(gl_FragCoord.xy), 0).rgba;

        // if we render the point circles
        #ifdef RENDER_POINT_CIRCLES
            if(tilesCol.r > 0 || tilesCol.g > 0 || tilesCol.b > 0){
                col = over(tilesCol, pointCircleCol);
            }    
        // if we do not render the point circles, we simply set the tile color
        #else
            col = tilesCol;
        #endif
    #endif

    // KDE overrides everything except Grid - debug
    #ifdef RENDER_KDE
        col = texelFetch(kdeTexture, ivec2(gl_FragCoord.xy), 0).rgba;
    #endif
    
    // NormalBuffer overrides everything except Grid - debug
    #ifdef RENDER_NORMAL_BUFFER
        col = vec4(texelFetch(normalsAndDepthTexture, ivec2(gl_FragCoord.xy),0).rgb, 1);
    #endif

    // DepthBuffer ocerrides everythin expert Grid - debug
    #ifdef RENDER_DEPTH_BUFFER
        col = vec4(abs(texelFetch(normalsAndDepthTexture, ivec2(gl_FragCoord.xy),0).a), 0, 0 ,1);
    #endif

    //add background color before grid
    col = col + vec4(backgroundColor,1) * (1.0f-col.a);

    //grid blends over everything
    #ifdef RENDER_GRID
        vec4 gridCol = texelFetch(gridTexture, ivec2(gl_FragCoord.xy), 0).rgba;
        col = over(gridCol, col);
    #endif

    colorTexture = col;
}


