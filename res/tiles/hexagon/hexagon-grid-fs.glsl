#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout (location = 0) out vec4 gridTexture;

uniform vec3 gridColor;
uniform float gridWidth;

//in screen space
in float tileSizeSS;
in vec2 tileCenterSS;
in float neighbourValues[6];
in vec2 accCoords;

// draws the grid, if gl_FragCoords is inside the gridWidth
// point naming:  
//	top 	2 <--- 1   
//	bottom 	1 ---> 2
//	counter clockwise
// inside = corner that is nearer to tile center
// outside = corner that is farther to tile center
void drawGrid(float distanceInside, float distanceInsideNorm, vec2 corner1, vec2 corner2, vec2 cornerInside1, vec2 cornerInside2){
	if(distanceInside <= gridWidth &&
		pointLeftOfLine(corner2, corner1, vec2(gl_FragCoord)) &&  
		pointLeftOfLine(cornerInside1, cornerInside2, vec2(gl_FragCoord)) && 
		pointLeftOfLine(corner1, cornerInside1, vec2(gl_FragCoord)) &&
		pointLeftOfLine(cornerInside2, corner2, vec2(gl_FragCoord)))
	{
		gridTexture = vec4(gridColor, distanceInsideNorm);
	}	
}

void main()
{

	/*
		-----
	  /	  |___\
	  \	   __ /
		-----

		upper horizontal line inside hexagon = tileSize  	(= distance to center corner points)
		lower horizontal line inside hexagon = tileSize / 2 (= x component of distance to top and bottom corner points)
		vertical line inside hexagon = vertical space / 2   (= y component of distance to top and bottom corner points)
	*/
	float tileSize_2 = tileSizeSS / 2.0f;
	float vertical_space_2 = sqrt(3) * tileSize_2;

	// from the center to the inside border edges the distance is tileSize/2 - gridWidth
	float tileSizeInside = tileSizeSS - gridWidth;
	float tileSizeInside_2 = tileSizeInside / 2.0f;
	float vertical_space_inside_2 = sqrt(3) * tileSizeInside_2;

	// from the center to the outside border edges the distance is tileSize/2 + gridWidth
	float tileSizeOutside = tileSizeSS + gridWidth;
	float tileSizeOutside_2 = tileSizeOutside / 2.0f;
	float vertical_space_outside_2 = sqrt(3) * tileSizeOutside_2;

	//hexagon tile corner points
 	vec2 leftBottomCorner = vec2(tileCenterSS + vec2(-tileSize_2, -vertical_space_2));     
	vec2 leftCenterCorner = vec2(tileCenterSS + vec2(-tileSizeSS, 0.0f));
	vec2 leftTopCorner = vec2(tileCenterSS + vec2(-tileSize_2, vertical_space_2));     
	vec2 rightBottomCorner = vec2(tileCenterSS + vec2(tileSize_2, -vertical_space_2));     
	vec2 rightCenterCorner = vec2(tileCenterSS + vec2(tileSizeSS, 0.0f));
	vec2 rightTopCorner = vec2(tileCenterSS + vec2(tileSize_2, vertical_space_2));

	//inside grid corner points
	vec2 leftBottomCornerInside = vec2(tileCenterSS + vec2(-tileSizeInside_2, -vertical_space_inside_2));     
	vec2 leftCenterCornerInside = vec2(tileCenterSS + vec2(-tileSizeInside, 0.0f));
	vec2 leftTopCornerInside = vec2(tileCenterSS + vec2(-tileSizeInside_2, vertical_space_inside_2));     
	vec2 rightBottomCornerInside = vec2(tileCenterSS + vec2(tileSizeInside_2, -vertical_space_inside_2));     
	vec2 rightCenterCornerInside = vec2(tileCenterSS + vec2(tileSizeInside, 0.0f));
	vec2 rightTopCornerInside = vec2(tileCenterSS + vec2(tileSizeInside_2, vertical_space_inside_2));

	// distance of fragment to inside border of grid
	//left top, left bottom, bottom, right bottom, right top, top
	float distancesInside[6];
	float distancesInsideNorm[6];

	distancesInside[0] = distancePointToLine(vec3(gl_FragCoord), vec3(leftCenterCornerInside, 0), vec3(leftTopCornerInside, 0));
	distancesInside[1] = distancePointToLine(vec3(gl_FragCoord), vec3(leftBottomCornerInside, 0), vec3(leftCenterCornerInside, 0));
	distancesInside[2] = (tileCenterSS.y - vertical_space_inside_2) - gl_FragCoord.y;
	distancesInside[3] = distancePointToLine(vec3(gl_FragCoord), vec3(rightCenterCornerInside, 0), vec3(rightBottomCornerInside, 0));
	distancesInside[4] = distancePointToLine(vec3(gl_FragCoord), vec3(rightTopCornerInside, 0), vec3(rightCenterCornerInside, 0));
	distancesInside[5] = gl_FragCoord.y - (tileCenterSS.y + vertical_space_inside_2);
	
	for(int i = 0; i < 6; i++){
		distancesInsideNorm[i] = distancesInside[i] / gridWidth;
	}

	//outside grid corner points
	vec2 leftBottomCornerOutside = vec2(tileCenterSS + vec2(-tileSizeOutside_2, -vertical_space_outside_2));     
	vec2 leftCenterCornerOutside = vec2(tileCenterSS + vec2(-tileSizeOutside, 0.0f));
	vec2 leftTopCornerOutside = vec2(tileCenterSS + vec2(-tileSizeOutside_2, vertical_space_outside_2));     
	vec2 rightBottomCornerOutside = vec2(tileCenterSS + vec2(tileSizeOutside_2, -vertical_space_outside_2));     
	vec2 rightCenterCornerOutside = vec2(tileCenterSS + vec2(tileSizeOutside, 0.0f));
	vec2 rightTopCornerOutside = vec2(tileCenterSS + vec2(tileSizeOutside_2, vertical_space_outside_2));

	// distance of fragment to outside border of grid
	//left top, left bottom, bottom, right bottom, right top, top
	float distancesOutside[6];
	float distancesOutsideNorm[6];

	distancesOutside[0] = distancePointToLine(vec3(gl_FragCoord), vec3(leftCenterCornerOutside, 0), vec3(leftTopCornerOutside, 0));
	distancesOutside[1] = distancePointToLine(vec3(gl_FragCoord), vec3(leftBottomCornerOutside, 0), vec3(leftCenterCornerOutside, 0));
	distancesOutside[2] = gl_FragCoord.y - (tileCenterSS.y - vertical_space_outside_2);
	distancesOutside[3] = distancePointToLine(vec3(gl_FragCoord), vec3(rightCenterCornerOutside, 0), vec3(rightBottomCornerOutside, 0));
	distancesOutside[4] = distancePointToLine(vec3(gl_FragCoord), vec3(rightTopCornerOutside, 0), vec3(rightCenterCornerOutside, 0));
	distancesOutside[5] = (tileCenterSS.y + vertical_space_outside_2) - gl_FragCoord.y;
	
	for(int i = 0; i < 6; i++){
		distancesOutsideNorm[i] = distancesOutside[i] / gridWidth;
	}

	//left top
	//inside
	drawGrid(distancesInside[0], distancesInsideNorm[0], leftTopCorner, leftCenterCorner, leftTopCornerInside, leftCenterCornerInside);
	//outside if neighbour is empty
	if(neighbourValues[0] == 0){
		drawGrid(distancesOutside[0], distancesOutsideNorm[0], leftTopCornerOutside, leftCenterCornerOutside, leftTopCorner, leftCenterCorner);	
	}

	//left bottom
	//inside
	drawGrid(distancesInside[1], distancesInsideNorm[1], leftCenterCorner, leftBottomCorner, leftCenterCornerInside, leftBottomCornerInside);
	//outside if neighbour is empty
	if(neighbourValues[1] == 0){
		drawGrid(distancesOutside[1], distancesOutsideNorm[1], leftCenterCornerOutside, leftBottomCornerOutside, leftCenterCorner, leftBottomCorner);	
	}

	//bottom
	//inside
	drawGrid(distancesInside[2], distancesInsideNorm[2], leftBottomCorner, rightBottomCorner, leftBottomCornerInside, rightBottomCornerInside);
	//outside if neighbour is empty
	if(neighbourValues[2] == 0){
		drawGrid(distancesOutside[2], distancesOutsideNorm[2], leftBottomCornerOutside, rightBottomCornerOutside, leftBottomCorner, rightBottomCorner);	
	}

	//right bottom
	//inside
	drawGrid(distancesInside[3], distancesInsideNorm[3], rightBottomCorner, rightCenterCorner, rightBottomCornerInside, rightCenterCornerInside);
	//outside if neighbour is empty
	if(neighbourValues[3] == 0){
		drawGrid(distancesOutside[3], distancesOutsideNorm[3], rightBottomCornerOutside, rightCenterCornerOutside, rightBottomCorner, rightCenterCorner);	
	}

	//right top
	//inside
	drawGrid(distancesInside[4], distancesInsideNorm[4], rightCenterCorner, rightTopCorner, rightCenterCornerInside, rightTopCornerInside);
	//outside if neighbour is empty
	if(neighbourValues[4] == 0){
		drawGrid(distancesOutside[4], distancesOutsideNorm[4], rightCenterCornerOutside, rightTopCornerOutside, rightCenterCorner, rightTopCorner);	
	}
	//top
	//inside
	drawGrid(distancesInside[5], distancesInsideNorm[5], rightTopCorner, leftTopCorner, rightTopCornerInside, leftTopCornerInside);
	//outside if neighbour is empty
	if(neighbourValues[5] == 0){
		drawGrid(distancesOutside[5], distancesOutsideNorm[5], rightTopCornerOutside, leftTopCornerOutside, rightTopCorner, leftTopCorner);	
	}

}