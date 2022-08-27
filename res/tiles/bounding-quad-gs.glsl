#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(points) in;
layout(triangle_strip, max_vertices = 4) out;

//[x,y]
uniform vec2 maxBounds_acc;
uniform vec2 minBounds_acc;

uniform mat4 modelViewProjectionMatrix;

#ifdef RENDER_TILES
	uniform int windowHeight;
	uniform int windowWidth;
	uniform float tileSize;
#endif


//if RENDER_HEXAGONS is defined RENDER_TILES is also defined -> we have windowHeight, windowWidth
#ifdef RENDER_HEXAGONS
	uniform vec2 rectSize;
#endif

out float tileSizeScreenSpace;
//[maxX,maxY,minX,minY]
out vec4 boundsScreenSpace;
out vec2 rectSizeScreenSpace;

/*
This geometry shader creates a quad which is of the size of 
maxBounds_acc - minBounds_acc

it is used in many different shader passes and uses defines to compute the currently needed values
*/
void main()
{

	#ifdef RENDER_TILES
		// bounding box in screen space
    	boundsScreenSpace = getScreenSpacePosOfRect(modelViewProjectionMatrix, maxBounds_acc, minBounds_acc, windowWidth, windowHeight);
		// tile size in screen space
		tileSizeScreenSpace = getScreenSpaceSize(modelViewProjectionMatrix, vec2(tileSize, 0.0f), windowWidth, windowHeight).x;
	#endif

	#ifdef RENDER_HEXAGONS
		// rectangle size in screen space
		rectSizeScreenSpace = getScreenSpaceSize(modelViewProjectionMatrix, rectSize, windowWidth, windowHeight);
	#endif

    // create bounding box geometry
	vec4 pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(maxBounds_acc[0],maxBounds_acc[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(minBounds_acc[0],maxBounds_acc[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(maxBounds_acc[0],minBounds_acc[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	pos = modelViewProjectionMatrix * (gl_in[0].gl_Position + vec4(minBounds_acc[0],minBounds_acc[1],0.0,0.0));
	gl_Position = pos;
	EmitVertex();

	EndPrimitive();
}