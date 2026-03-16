


#pragma include "misc.glsl"

// varying vec4	vUV;
// varying vec3	vNormal;
// varying vec4	vCameraCoordXYZandClipZ;

in FragmentIn
{
	vec3 normal;
	//vec4 uv;
	vec4 cameraPosAndClipZ;
	//float clipZ;
} In;

layout(location = 0) out vec4 out_albedo;
layout(location = 1) out vec4 out_normal;
layout(location = 2) out vec4 out_specular;
layout(location = 3) out vec4 out_posAndClipz;

uniform float	uMaterialID;

void main()
{
	out_albedo = vec4(0.3, 0, 0, uMaterialID);
	out_normal = vec4(normalize(In.normal), 0);	
	out_specular = vec4(0, 0, 0, 0);	
	out_posAndClipz = In.cameraPosAndClipZ;
		
	gl_FragDepth = calculate_depth(In.cameraPosAndClipZ.w);
}
