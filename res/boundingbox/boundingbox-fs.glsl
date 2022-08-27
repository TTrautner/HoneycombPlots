#version 400

out vec4 fragColor;
in vec4 gPatchDistance;

void main()
{
	vec4 d = fwidth(gPatchDistance);
    vec4 s = smoothstep(vec4(0.0), d*2.5, gPatchDistance);
	float edgeFactor = min(min(s.x, s.y), min(s.z,s.w));

	if (edgeFactor >= 1.0)
		discard;

	if(gl_FrontFacing)
	{
		fragColor = vec4(0.5, 0.5, 0.5, (1.0-edgeFactor)*0.95);
	}
	else
	{
		fragColor = vec4(0.5, 0.5, 0.5, (1.0-edgeFactor)*0.7);
	}
}