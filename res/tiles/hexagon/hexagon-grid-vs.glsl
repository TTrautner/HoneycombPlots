#version 450
#extension GL_ARB_shading_language_include : require
#include "/defines.glsl"

layout(location = 0) in float xValue;

uniform float horizontal_space;
uniform float vertical_space;
uniform float tileSize;
uniform int num_cols;
uniform vec2 minBounds_Off;

out VS_OUT {
    vec2 accTexPosition;
} vs_out;

void main()
{
	//calculate position of hexagon center - in double height coordinates!
	//https://www.redblobgames.com/grids/hexagons/#coordinates-doubled
	float row;
	float col = mod(gl_VertexID,num_cols);

	if(mod(col,2) == 0){
		row = floor(gl_VertexID/num_cols) * 2;
	}
	else{
		row = (floor(gl_VertexID/num_cols) * 2) - 1;
	}
	
	//position of hexagon in accumulate texture (offset coordinates)
	vs_out.accTexPosition = vec2(col, floor(gl_VertexID/num_cols));

	gl_Position =  vec4(col * horizontal_space + minBounds_Off.x + tileSize, row * vertical_space/2.0f + minBounds_Off.y + vertical_space, 0.0f, 1.0f);
}