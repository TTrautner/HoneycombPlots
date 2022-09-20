#version 450
#include "/defines.glsl"
#include "/globals.glsl"
#include "/hex/globals.glsl"

//--in
layout(pixel_center_integer) in vec4 gl_FragCoord;

layout(std430, binding = 0) buffer tileNormalsBuffer
{
    int tileNormals[];
};

layout(std430, binding = 1) buffer valueMaxBuffer
{
    uint maxAccumulate;
    uint maxPointAlpha;
};

in vec4 boundsScreenSpace;
in vec2 rectSizeScreenSpace;
in float tileSizeScreenSpace;

uniform sampler2D accumulateTexture;
uniform sampler2D tilesDiscrepancyTexture;

// 1D color map parameters
uniform sampler1D colorMapTexture;
uniform int textureWidth;

//min = 0
uniform int maxTexCoordX;
uniform int maxTexCoordY;

uniform int max_rect_col;
uniform int max_rect_row;

uniform float bufferAccumulationFactor;
uniform float tileHeightMult;
uniform float densityMult;
uniform float borderWidth;
uniform bool showBorder;
uniform bool invertPyramid;
uniform float blendRange;

uniform vec3 tileColor;
uniform float aaoScaling;

uniform float diamondCutOutlineWidth;
uniform float diamondCutOutlineDarkening;

//Lighting----------------------
uniform vec3 lightPos; 
uniform vec3 viewPos; //is vec3(0,0,0) because we work in viewspace
uniform vec3 lightColor;

uniform int windowWidth;
uniform int windowHeight;
//----------------------
//--out
layout(location = 0) out vec4 hexTilesTexture;
layout (location = 1) out vec4 normalsAndDepthTexture;


/*
returns true if the fragment is not in the plane-pyramid intersection region
p       = fragment that is checked
other   = corner points of the intersection region
*/
bool pointInBorder(vec3 p, vec3 leftBottomInside, vec3 leftCenterInside, vec3 leftTopInside, vec3 rightBottomInside, vec3 rightCenterInside, vec3 rightTopInside){
    
    return pointLeftOfLine(vec2(leftTopInside), vec2(leftCenterInside), vec2(p))
            || pointLeftOfLine(vec2(leftCenterInside), vec2(leftBottomInside), vec2(p))
            || pointLeftOfLine(vec2(leftBottomInside), vec2(rightBottomInside), vec2(p))
            || pointLeftOfLine(vec2(rightBottomInside), vec2(rightCenterInside),vec2(p))
            || pointLeftOfLine(vec2(rightCenterInside), vec2(rightTopInside), vec2(p))
            || pointLeftOfLine(vec2(rightTopInside), vec2(leftTopInside), vec2(p));
}

void main()
{

	// transform fragCoordinates to view-space
	vec3 fragViewSpace = vec3((2.0 * gl_FragCoord.xy) / vec2(windowWidth, windowHeight) - 1.0, 2.0 * gl_FragCoord.z - 1.0);

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

    // get value from accumulate texture
    float hexValue = texelFetch(accumulateTexture, ivec2(hex.x, hex.y), 0).r;

    // we don't want to render empty hexs
    if (hexValue < 0.00000001)
    {
        discard;
    }

    // read from SSBO and convert back to float: --------------------------------------------------------------------
    // - https://www.khronos.org/registry/OpenGL-Refpages/gl4/html/intBitsToFloat.xhtml
    // +1 because else we cannot map the maximum value itself
    float floatMaxAccumulate = uintBitsToFloat(maxAccumulate) + 1;

    //color hexs according to index
    hexTilesTexture = vec4(float(hex.x / float(maxTexCoordX)), float(hex.y / float(maxTexCoordY)), 0.0f, 1.0f);

// color hex according to value
#ifdef COLORMAP
    int colorTexelCoord = mapInterval(hexValue, 0, floatMaxAccumulate, textureWidth);
    hexTilesTexture.rgb = texelFetch(colorMapTexture, colorTexelCoord, 0).rgb;
#endif 

	// horizontal and vertical space between hexagons (https://www.redblobgames.com/grids/hexagons/)
    float horizontal_space = tileSizeScreenSpace * 1.5f;
	float vertical_space = sqrt(3)*tileSizeScreenSpace;

	// vertical offset for each second row of hexagon grid
    float vertical_offset = mod(hex.x, 2) == 0 ? vertical_space : vertical_space/2.0f;

	// x,y of center of tile
    vec2 tileCenter2D = vec2(hex.x * horizontal_space + boundsScreenSpace[2] + tileSizeScreenSpace, hex.y * vertical_space + boundsScreenSpace[3] + vertical_offset); 

#ifdef RENDER_MONOCHROME_TILES
	hexTilesTexture.rgb = tileColor;
#endif

#ifdef RENDER_DISCREPANCY
    //discrepancy is set as opacity
    float tilesDiscrepancy = texelFetch(tilesDiscrepancyTexture, ivec2(hex.x, hex.y), 0).r;
    hexTilesTexture *= tilesDiscrepancy;
#else
	// make transparency dependent on the "thickness" of a tile
	hexTilesTexture.a = hexValue/floatMaxAccumulate;
#endif

// REGRESSION PLANE------------------------------------------------------------------------------------------------
    vec4 tileNormal = vec4(0.0f,0.0f,0.0f,1.0f);
    vec3 lightingNormal = vec3(0.0f,0.0f,0.0f);
    vec3 fragmentPos = vec3(gl_FragCoord);
    float kdeHeight = 0.0f;

	//Corner Of Hexagon (z=0)
    vec3 leftBottomCorner = vec3(tileCenter2D + vec2(-tileSizeScreenSpace/2.0f, -vertical_space/2.0f), 0.0f);     
    vec3 leftCenterCorner = vec3(tileCenter2D + vec2(-tileSizeScreenSpace, 0.0f), 0.0f);
    vec3 leftTopCorner = vec3(tileCenter2D + vec2(-tileSizeScreenSpace/2.0f, vertical_space/2.0f), 0.0f);     
    vec3 rightBottomCorner = vec3(tileCenter2D + vec2(tileSizeScreenSpace/2.0f, -vertical_space/2.0f), 0.0f);     
    vec3 rightCenterCorner = vec3(tileCenter2D + vec2(tileSizeScreenSpace, 0.0f), 0.0f);
    vec3 rightTopCorner = vec3(tileCenter2D + vec2(tileSizeScreenSpace/2.0f, vertical_space/2.0f), 0.0f); 


    #ifdef RENDER_TILE_NORMALS
        // get accumulated tile normals from buffer
        for(int i = 0; i < 4; i++){
            tileNormal[i] = float(tileNormals[int((hex.x*(maxTexCoordY+1) + hex.y) * 5 + i)]);
        }
        tileNormal /= bufferAccumulationFactor;// get original value after accumulation

        // get kdeHeight from buffer
        kdeHeight = float(tileNormals[int((hex.x*(maxTexCoordY+1) + hex.y) * 5 + 4)]);
        kdeHeight /= bufferAccumulationFactor; // get original value after accumulation
        kdeHeight /= densityMult; //remove multiplyer from height
        kdeHeight /= tileNormal.w; //normalize according to number of fragments in tile

        // LIGHTING NORMAL ------------------------
        lightingNormal = normalize(vec3(tileNormal.x, tileNormal.y, tileNormal.w));
        //-----------------------------------------
 
        //height at tile center
        float pyramidHeight = kdeHeight * tileHeightMult;
        if(invertPyramid){
            pyramidHeight *= -1;
        }

        vec3 tileCenter3D;
        vec3 pyramidTop = vec3(tileCenter2D, pyramidHeight);

        if(showBorder){
            // BORDER-------------------------------------
            // 1) get lowest corner point
            float heightLeftBottomCorner = getHeightOfPointOnSurface(vec2(leftBottomCorner), pyramidTop, lightingNormal);
            float heightLeftCenterCorner = getHeightOfPointOnSurface(vec2(leftCenterCorner), pyramidTop, lightingNormal);
            float heightLeftTopCorner = getHeightOfPointOnSurface(vec2(leftTopCorner), pyramidTop, lightingNormal);
            float heightRightBottomCorner = getHeightOfPointOnSurface(vec2(rightBottomCorner), pyramidTop, lightingNormal);
            float heightRightCenterCorner = getHeightOfPointOnSurface(vec2(rightCenterCorner), pyramidTop, lightingNormal);
            float heightRightTopCorner = getHeightOfPointOnSurface(vec2(rightTopCorner), pyramidTop, lightingNormal);

            float heightOffset;
            //we need a small offset, because we cannot set an inside corner point to height 0
            //this would lead to the effect that the regresseion plane is overlapping with one of the border planes
            //this on the other hand pused all inside corner points to the center of the tile, because there is no more cutting plane
            //between regression plane and pyramid.
            //if thats the case, we can not longer compute if a point is in the border, because there are no more line between the inside corner points
            float offset = 0.01f;
            if(invertPyramid){
                float maxLeftCorner = max(max(heightLeftBottomCorner, heightLeftCenterCorner), heightLeftTopCorner);
                float maxRightCorner = max(max(heightRightBottomCorner, heightRightCenterCorner), heightRightTopCorner);
                float maxHeightCorner = max(maxLeftCorner, maxRightCorner);

               //we need to clamp the pyramidHeight to pyramidHeight + maxHeightCorner*-1 to avoid that the maximum Height Corner is positive
                if(maxHeightCorner > -offset){
                    float maxCornerNegative = maxHeightCorner > 0 ? maxHeightCorner * (-1) - offset : maxHeightCorner - offset;
                    pyramidHeight += maxCornerNegative;
                    pyramidTop = vec3(tileCenter2D, pyramidHeight);
                    maxHeightCorner = -offset;
                }
                heightOffset = pyramidHeight - maxHeightCorner;
            }
            else{
                float minLeftCorner = min(min(heightLeftBottomCorner, heightLeftCenterCorner), heightLeftTopCorner);
                float minRightCorner = min(min(heightRightBottomCorner, heightRightCenterCorner), heightRightTopCorner);
                float minHeightCorner = min(minLeftCorner, minRightCorner);
                //we need to clamp the pyramidHeight to pyramidHeight + abs(minHeightCorner) to avoid that the minimum Height Corner is negative
                if(minHeightCorner < offset){
                    float minCornerAbs = abs(minHeightCorner) + offset;
                    pyramidHeight += minCornerAbs;
                    pyramidTop = vec3(tileCenter2D, pyramidHeight);
                    minHeightCorner = offset;
                }
                heightOffset = pyramidHeight - minHeightCorner;
            }

            // 2) get z value of regression plane center by multiplying pyramidHeight-minHeightCorner with borderWidth and then adding minHeightCorner again
            // get regression plane
            float tileCenterZ = (pyramidHeight - heightOffset) * borderWidth + heightOffset;
            
            tileCenter3D = vec3(tileCenter2D, tileCenterZ);
            
            // 3) get intersections of plane with pyramid
            // lineDir = normalize(pyramidTop - Corner)

            vec3 leftBottomInside = linePlaneIntersection(lightingNormal, tileCenter3D, normalize(pyramidTop - leftBottomCorner), pyramidTop);        
            vec3 leftCenterInside = linePlaneIntersection(lightingNormal, tileCenter3D, normalize(pyramidTop - leftCenterCorner), pyramidTop);        
            vec3 leftTopInside = linePlaneIntersection(lightingNormal, tileCenter3D, normalize(pyramidTop - leftTopCorner), pyramidTop);        
            vec3 rightBottomInside = linePlaneIntersection(lightingNormal, tileCenter3D, normalize(pyramidTop - rightBottomCorner), pyramidTop);        
            vec3 rightCenterInside = linePlaneIntersection(lightingNormal, tileCenter3D, normalize(pyramidTop - rightCenterCorner), pyramidTop);        
            vec3 rightTopInside = linePlaneIntersection(lightingNormal, tileCenter3D, normalize(pyramidTop - rightTopCorner), pyramidTop);        

            //check if fragment is in intersection region of on pyramid side --------------------------------------------
            if(pointInBorder(fragmentPos, leftBottomInside, leftCenterInside, leftTopInside, rightBottomInside, rightCenterInside, rightTopInside)){
                float colorNormalDepth[7];

                //check which side
                //left top
                if(pointLeftOfLine(vec2(leftTopInside), vec2(leftCenterInside), vec2(fragmentPos)) 
                && pointLeftOfLine(vec2(leftCenterInside),vec2(leftCenterCorner),vec2(fragmentPos))
                && pointLeftOfLine(vec2(leftTopCorner),vec2(leftTopInside),vec2(fragmentPos))){
                    
                    colorNormalDepth = calculateBorderColor(fragmentPos, lightingNormal, leftCenterCorner, leftCenterInside, leftBottomInside,
                        leftTopCorner, leftTopInside, rightTopInside, tileCenter3D, blendRange, hexTilesTexture, lightColor, lightPos, viewPos, windowWidth, windowHeight, tileSizeScreenSpace);
                }
                //left bottom
                else if(pointLeftOfLine(vec2(leftCenterInside), vec2(leftBottomInside), vec2(fragmentPos)) 
                && pointLeftOfLine(vec2(leftCenterCorner),vec2(leftCenterInside),vec2(fragmentPos)) 
                && pointLeftOfLine(vec2(leftBottomInside),vec2(leftBottomCorner),vec2(fragmentPos))){

                    colorNormalDepth = calculateBorderColor(fragmentPos, lightingNormal, leftBottomCorner, leftBottomInside, rightBottomInside,
                        leftCenterCorner, leftCenterInside, leftTopInside, tileCenter3D, blendRange, hexTilesTexture, lightColor, lightPos, viewPos, windowWidth, windowHeight, tileSizeScreenSpace);
                }
                //bottom
                else if(pointLeftOfLine(vec2(leftBottomInside), vec2(rightBottomInside), vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightBottomInside), vec2(rightBottomCorner), vec2(fragmentPos))
                && pointLeftOfLine(vec2(leftBottomCorner),vec2(leftBottomInside),vec2(fragmentPos))){
                    
                    colorNormalDepth = calculateBorderColor(fragmentPos, lightingNormal, rightBottomCorner, rightBottomInside, rightCenterInside,
                        leftBottomCorner, leftBottomInside, leftCenterInside, tileCenter3D, blendRange, hexTilesTexture, lightColor, lightPos, viewPos, windowWidth, windowHeight, tileSizeScreenSpace);
                } 
                //right bottom
                else if(pointLeftOfLine(vec2(rightBottomInside),vec2(rightCenterInside), vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightBottomCorner), vec2(rightBottomInside), vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightCenterInside),vec2(rightCenterCorner),vec2(fragmentPos))){

                    colorNormalDepth = calculateBorderColor(fragmentPos, lightingNormal, rightCenterCorner, rightCenterInside, rightTopInside,
                        rightBottomCorner, rightBottomInside, leftBottomInside, tileCenter3D, blendRange, hexTilesTexture, lightColor, lightPos, viewPos, windowWidth, windowHeight, tileSizeScreenSpace);
                }
                //right top
                else if(pointLeftOfLine(vec2(rightCenterInside),vec2(rightTopInside), vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightCenterCorner),vec2(rightCenterInside), vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightTopInside),vec2(rightTopCorner),vec2(fragmentPos))){

                    colorNormalDepth = calculateBorderColor(fragmentPos, lightingNormal, rightTopCorner, rightTopInside, leftTopInside, 
                        rightCenterCorner, rightCenterInside, rightBottomInside, tileCenter3D, blendRange, hexTilesTexture, lightColor, lightPos, viewPos, windowWidth, windowHeight, tileSizeScreenSpace);
                }
                //top
                else if(pointLeftOfLine(vec2(rightTopInside), vec2(leftTopInside), vec2(fragmentPos))
                && pointLeftOfLine(vec2(leftTopInside), vec2(leftTopCorner), vec2(fragmentPos))
                && pointLeftOfLine(vec2(rightTopCorner), vec2(rightTopInside), vec2(fragmentPos))){
                    
                    colorNormalDepth = calculateBorderColor(fragmentPos, lightingNormal, leftTopCorner, leftTopInside, leftCenterInside,
                        rightTopCorner, rightTopInside, rightCenterInside, tileCenter3D, blendRange, hexTilesTexture, lightColor, lightPos, viewPos, windowWidth, windowHeight, tileSizeScreenSpace);
                }

                hexTilesTexture.r = colorNormalDepth[0];
                hexTilesTexture.g = colorNormalDepth[1];
                hexTilesTexture.b = colorNormalDepth[2];

                normalsAndDepthTexture.r = colorNormalDepth[3];
                normalsAndDepthTexture.g = colorNormalDepth[4];
                normalsAndDepthTexture.b = colorNormalDepth[5];
                normalsAndDepthTexture.a = colorNormalDepth[6];
            }
            //point is on the inside
            else{                   
                // fragment height
                fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), tileCenter3D, lightingNormal);          

				// PHONG LIGHTING 
                hexTilesTexture.rgb = calculatePhongLighting(lightColor, lightPos, fragViewSpace, lightingNormal, viewPos) * hexTilesTexture.rgb;

                //write into normals&depth buffer
                normalsAndDepthTexture = vec4(lightingNormal, fragmentPos.z);   
				

			#ifdef RENDER_DIAMONDCUT_OUTLINE
				float d_lowLeft = length(cross(fragmentPos.xyz - leftBottomInside, leftCenterInside - leftBottomInside)) / length(leftCenterInside - leftBottomInside);
				float d_topLeft = length(cross(fragmentPos.xyz - leftTopInside, leftCenterInside - leftTopInside)) / length(leftCenterInside - leftTopInside);

				float d_lowRight = length(cross(fragmentPos.xyz - rightBottomInside, rightCenterInside - rightBottomInside)) / length(rightCenterInside - rightBottomInside);
				float d_topRight = length(cross(fragmentPos.xyz - rightTopInside, rightCenterInside - rightTopInside)) / length(rightCenterInside - rightTopInside);

				float d_top = length(cross(fragmentPos.xyz - leftTopInside, rightTopInside - leftTopInside)) / length(rightTopInside - leftTopInside);
				float d_bottom = length(cross(fragmentPos.xyz - rightBottomInside, leftBottomInside - rightBottomInside)) / length(leftBottomInside - rightBottomInside);

				float d = min(min(d_lowLeft,d_topLeft), min(min(d_lowRight,d_topRight),  min(d_top,d_bottom)));

				if(d <diamondCutOutlineWidth){
					hexTilesTexture.rgb = mix(hexTilesTexture.rgb, hexTilesTexture.rgb*diamondCutOutlineDarkening, smoothstep(0.0f, 1.0f, d/diamondCutOutlineWidth));
				}else if (d >=diamondCutOutlineWidth && d < diamondCutOutlineWidth*2){
					hexTilesTexture.rgb = mix(hexTilesTexture.rgb*diamondCutOutlineDarkening, hexTilesTexture.rgb, smoothstep(0.0f, 1.0f, (d-diamondCutOutlineWidth)/diamondCutOutlineWidth));
				}
			#endif

            }
        }
        else{

            //regression plane center is on height of pyramid
            tileCenter3D = pyramidTop;

            // fragemnt height
            fragmentPos.z = getHeightOfPointOnSurface(vec2(fragmentPos), tileCenter3D, lightingNormal);   

            // PHONG LIGHTING 
            hexTilesTexture.rgb = calculatePhongLighting(lightColor, lightPos, fragViewSpace, lightingNormal, viewPos) * hexTilesTexture.rgb; 

            //write into normals&depth buffer
            normalsAndDepthTexture = vec4(lightingNormal, fragmentPos.z);      
        }

		// make the colors fade towards the border
		//hexTilesTexture.rgb = mix(hexTilesTexture.rgb, vec3(0,0,0), 1.0f - length(tileCenter2D-fragmentPos.xy) / tileSizeScreenSpace);

    #endif

	// Analytical Ambien Occlusion ===============================================================================================================================================================================================================
	#ifdef RENDER_ANALYTICAL_AO	
	
		// get neighbouring hex-coordinates --------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		vec2 hex_below	= hex + vec2( 0, -1);
		vec2 hex_above	= hex + vec2( 0,  1);

		vec2 hex_belowRight, hex_aboveRight, hex_aboveLeft, hex_belowLeft;

		if(mod(hex.x,2) == 0){
			hex_belowRight	= hex + vec2( 1,  0);
			hex_aboveRight	= hex + vec2( 1,  1);
			hex_aboveLeft	= hex + vec2(-1,  1);
			hex_belowLeft	= hex + vec2(-1,  0);
		}else{
			hex_belowRight	= hex + vec2( 1,  -1);
			hex_aboveRight	= hex + vec2( 1,  0);
			hex_aboveLeft	= hex + vec2(-1,  0);
			hex_belowLeft	= hex + vec2(-1,  -1);
		}

		// get accumulated value from neighbouring hex-tiles ---------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		float hexValue_below = texelFetch(accumulateTexture, ivec2(hex_below), 0).r;
		float hexValue_above = texelFetch(accumulateTexture, ivec2(hex_above), 0).r;
		float hexValue_belowRight = texelFetch(accumulateTexture, ivec2(hex_belowRight), 0).r;
		float hexValue_aboveRight = texelFetch(accumulateTexture, ivec2(hex_aboveRight), 0).r;
		float hexValue_aboveLeft = texelFetch(accumulateTexture, ivec2(hex_aboveLeft), 0).r;
		float hexValue_belowLeft = texelFetch(accumulateTexture, ivec2(hex_belowLeft), 0).r;

		// calculate analytical ambient occlusiont -------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------------
		// check: https://www.shadertoy.com/view/WtSfWK and https://www.ii.uni.wroc.pl/~anl/cgfiles/TotalCompendium.pdf (page 41 bottom)

		vec3 fragPos = vec3(vec2(gl_FragCoord), hexValue);
		vec3 defaultNormal = vec3(0.0f, 0.0f, 1.0f);	

		float occ = 0.0f;

		if(hexValue_belowLeft > hexValue)	// lower left hex
			occ += occlusionQuad(fragPos, defaultNormal, vec3(leftBottomCorner.xy, hexValue), vec3(leftCenterCorner.xy, hexValue), vec3(leftCenterCorner.xy, hexValue_belowLeft), vec3(leftBottomCorner.xy, hexValue_belowLeft));

		if(hexValue_aboveLeft > hexValue)	// upper left hex
			occ += occlusionQuad(fragPos, defaultNormal, vec3(leftCenterCorner.xy, hexValue),	vec3(leftTopCorner.xy, hexValue), vec3(leftTopCorner.xy, hexValue_aboveLeft), vec3(leftCenterCorner.xy, hexValue_aboveLeft));

		if(hexValue_above > hexValue)		// top hex
			occ += occlusionQuad(fragPos, defaultNormal, vec3(leftTopCorner.xy, hexValue), vec3(rightTopCorner.xy, hexValue),	vec3(rightTopCorner.xy, hexValue_above), vec3(leftTopCorner.xy, hexValue_above));

		if(hexValue_aboveRight > hexValue)	// upper right hex
			occ += occlusionQuad(fragPos, defaultNormal, vec3(rightTopCorner.xy, hexValue), vec3(rightCenterCorner.xy, hexValue),	vec3(rightCenterCorner.xy, hexValue_aboveRight), vec3(rightTopCorner.xy, hexValue_aboveRight));

		if(hexValue_belowRight > hexValue)	// lower right hex
			occ += occlusionQuad(fragPos, defaultNormal, vec3(rightCenterCorner.xy, hexValue), vec3(rightBottomCorner.xy, hexValue), vec3(rightBottomCorner.xy, hexValue_belowRight),	vec3(rightCenterCorner.xy, hexValue_belowRight));

		if(hexValue_below > hexValue)		// lower hex
			occ += occlusionQuad(fragPos, defaultNormal, vec3(rightBottomCorner.xy, hexValue), vec3(leftBottomCorner.xy, hexValue), vec3(leftBottomCorner.xy, hexValue_below), vec3(rightBottomCorner.xy, hexValue_below));

		// apply occlusion to tile color
		hexTilesTexture.rgb *= 1.0f-occ*aaoScaling;
	#endif
}
