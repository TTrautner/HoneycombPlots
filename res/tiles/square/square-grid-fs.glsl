#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout (location = 0) out vec4 gridTexture;

uniform vec3 gridColor;
uniform float gridWidth;

in float tileSizeSS;
in vec2 tileCenterSS;
//left,bottom,right,top
in vec4 neighbourValues;

// draws the grid, if gl_FragCoords is inside the gridWidth
// point naming:  
//	top 	2 <--- 1   
//	bottom 	1 ---> 2
//	counter clockwise
// inside = corner that is nearer to tile center
// outside = corner that is farther to tile center
void drawGrid(float distanceToGrid, float distanceToGridNorm, vec2 cornerOutside1, vec2 cornerOutside2, vec2 cornerInside1, vec2 cornerInside2){
	
	if(distanceToGrid <= gridWidth && 
		pointLeftOfLine(cornerOutside1, cornerInside1, vec2(gl_FragCoord)) &&
		pointLeftOfLine(cornerInside2, cornerOutside2, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(gridColor, distanceToGridNorm);
	}	
}

void main()
{

	// from the center to the tile edges the distance is half the tile size
	float tileSize_2 = tileSizeSS / 2.0f;
	// from the center to the inside border edges the distance is tileSize/2 - gridWidth
	float tileSizeInside_2 = tileSize_2 - gridWidth;
	// from the center to the outside border edges the distance is tileSize/2 + gridWidth
	float tileSizeOutside_2 = tileSize_2 + gridWidth;

	//square tile corners
	vec2 leftBottomCorner = tileCenterSS - tileSize_2;
    vec2 leftTopCorner = vec2(tileCenterSS.x - tileSize_2, tileCenterSS.y + tileSize_2);
    vec2 rightBottomCorner = vec2(tileCenterSS.x + tileSize_2, tileCenterSS.y - tileSize_2);
    vec2 rightTopCorner = tileCenterSS + tileSize_2;

	//inside grid corner points
	vec2 leftBottomCornerInside = tileCenterSS - tileSizeInside_2;
    vec2 leftTopCornerInside = vec2(tileCenterSS.x - tileSizeInside_2, tileCenterSS.y + tileSizeInside_2);
    vec2 rightBottomCornerInside = vec2(tileCenterSS.x + tileSizeInside_2, tileCenterSS.y - tileSizeInside_2);
    vec2 rightTopCornerInside = tileCenterSS + tileSizeInside_2;

	//left, bottom, right, top
	vec4 distancesInside = vec4(leftBottomCornerInside.x - gl_FragCoord.x, 
								leftBottomCornerInside.y - gl_FragCoord.y, 
								gl_FragCoord.x - rightBottomCornerInside.x, 
								gl_FragCoord.y - leftTopCornerInside.y);
	vec4 distancesInsideNorm = distancesInside / gridWidth;

	//outside
	vec2 leftBottomCornerOutside = tileCenterSS - tileSizeOutside_2;
    vec2 leftTopCornerOutside = vec2(tileCenterSS.x - tileSizeOutside_2, tileCenterSS.y + tileSizeOutside_2);
    vec2 rightBottomCornerOutside = vec2(tileCenterSS.x + tileSizeOutside_2, tileCenterSS.y - tileSizeOutside_2);
    vec2 rightTopCornerOutside = tileCenterSS + tileSizeOutside_2;

	//left, bottom, right, top
	vec4 distancesOutside = vec4(gl_FragCoord.x - leftBottomCornerOutside.x,
   								gl_FragCoord.y - leftBottomCornerOutside.y,
 								rightBottomCornerOutside.x - gl_FragCoord.x,
								leftTopCornerOutside.y - gl_FragCoord.y);
	vec4 distancesOutsideNorm = distancesOutside / gridWidth;

	//left
	//inside
	drawGrid(distancesInside.x, distancesInsideNorm.x, leftTopCorner, leftBottomCorner, leftTopCornerInside, leftBottomCornerInside);
		
	//outside if the left neighbour is empty
	if(neighbourValues.x == 0){
		drawGrid(distancesOutside.x, distancesOutsideNorm.x, leftTopCornerOutside, leftBottomCornerOutside, leftTopCorner, leftBottomCorner);
	}

	//bottom
	//inside
	drawGrid(distancesInside.y, distancesInsideNorm.y, leftBottomCorner, rightBottomCorner, leftBottomCornerInside, rightBottomCornerInside);

	//outside if bottom neghbour is empty
	if(neighbourValues.y == 0){
		drawGrid(distancesOutside.y, distancesOutsideNorm.y, leftBottomCornerOutside, rightBottomCornerOutside, leftBottomCorner, rightBottomCorner);
	}

	//right
	//inside 
	drawGrid(distancesInside.z, distancesInsideNorm.z, rightBottomCorner, rightTopCorner, rightBottomCornerInside, rightTopCornerInside);
	//outside if right neighbour is empty
	if(neighbourValues.z == 0){
		drawGrid(distancesOutside.z, distancesOutsideNorm.z, rightBottomCornerOutside, rightTopCornerOutside, rightBottomCorner, rightTopCorner);
	}
	
	//top
	//inside
	drawGrid(distancesInside.w, distancesInsideNorm.w, rightTopCorner, leftTopCorner, rightTopCornerInside, leftTopCornerInside);

	//outside if top neighbour is empty
	if(neighbourValues.w == 0){
		drawGrid(distancesOutside.w, distancesOutsideNorm.w, rightTopCornerOutside, leftTopCornerOutside, rightTopCorner, leftTopCorner);
	}
}