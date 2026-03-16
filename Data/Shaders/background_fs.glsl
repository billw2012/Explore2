

#pragma include "misc.glsl"

//in vec4	vCameraCoordXYZandClipZ;

in FragmentIn
{
	vec4 posAndClipz;
} In;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_specular;
layout(location = 3) out vec4 out_posAndClipz;


void main()
{
	out_albedo = vec4(0, 0, 0, NO_LIGHTING);
	out_normal = vec4(0, 0, 0, 0);	
	out_specular = vec4(0, 0, 0, 0);	
	out_posAndClipz = In.posAndClipz;
	
	gl_FragDepth = calculate_depth(In.posAndClipz.w);
}
