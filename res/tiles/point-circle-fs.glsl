#version 450
#include "/defines.glsl"
#include "/globals.glsl"

layout(pixel_center_integer) in vec4 gl_FragCoord;
layout (location = 0) out vec4 pointCircleTexture;
layout (location = 1) out vec4 kdeTexture;

const float PI = 3.14159265359;

uniform float sigma2;			
uniform float pointCircleRadius;
uniform float kdeRadius;
uniform float radiusMult;
uniform float densityMult;

in vec2 isCoords;

uniform vec3 pointColor;

float gaussKernel(float d)
{
    return 1 / (sigma2 * sqrt(2 * PI)) * exp(-(pow(d, 2) / (2 * sigma2)));
}


void main()
{
	float fragDistanceFromCenter;
	#ifdef RENDER_POINT_CIRCLES
		fragDistanceFromCenter = length(isCoords);

		// if the kdeRadius is bigger than the pointCircleRadius the imposter square has the size of kdeRadius
		// therefore we need to adjust the fragDistanceFromCenter which is in imposter coordinates
		if(pointCircleRadius < kdeRadius){
			fragDistanceFromCenter /= pointCircleRadius / kdeRadius;
		}

		// create circle
		if (fragDistanceFromCenter > 1) {
			pointCircleTexture = vec4(0,0,0,0);
		}
		else{
			float blendRadius = smoothstep(0.5, 1.0, fragDistanceFromCenter);

			// fade point color to outside 
			pointCircleTexture = vec4(pointColor, 1.0f-blendRadius);

			//OpenGL expects premultiplied alpha values
			pointCircleTexture.rgb *= pointCircleTexture.a;
		}
	#endif

	#if defined(RENDER_KDE) || defined(RENDER_TILE_NORMALS)
		fragDistanceFromCenter = length(isCoords);

		// if the kdeRadius is smaller than the pointCircleRadius the imposter square has the size of pointCircleRadius
		// therefore we need to adjust the fragDistanceFromCenter which is in imposter coordinates
		if(pointCircleRadius > kdeRadius){
			fragDistanceFromCenter /= kdeRadius / pointCircleRadius;
		}

		// create circle
		if (fragDistanceFromCenter > 1) {
			kdeTexture = vec4(0,0,0,0);
		}
		else{
			// kde value is computed using a gauss kernel
			// and is then multiplyed with densityMult (used adjsutable parameter) to increase or decrease it
			kdeTexture = vec4(gaussKernel(fragDistanceFromCenter*kdeRadius*radiusMult)*densityMult, 0.0f, 0.0f, 1.0f);
		}
	#endif
}