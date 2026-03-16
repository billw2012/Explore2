

uniform mat4	uModelViewProj;
uniform mat4	uModelView;
uniform float	uBlendFactor;

layout(location = 0) in vec3 in_vertex1;
layout(location = 1) in vec3 in_vertex0;

layout(location = 2) in vec4 in_textureUVs;
//layout(location = 2, component = 0) in vec2 in_textureUV;
//layout(location = 2, component = 2) in vec2 in_parentTextureUV;

layout(location = 3) in vec4 in_normalUVs;
// layout(location = 3, component = 0) in vec2 in_normalUV;
// layout(location = 3, component = 2) in vec2 in_parentNormalUV;


out FragmentIn
{
	vec2 textureUV;
	vec2 parentTextureUV;
	vec2 normalUV;
	vec2 parentNormalUV;
	vec4 cameraPosAndClipZ;
	//float clipZ;
} Out;

// varying vec4	vTextureUV;
// varying vec4	vNormalUV;
// varying vec4	vCameraCoordXYZandClipZ;
	

void main()
{
	Out.textureUV = in_textureUVs.xy;
	Out.parentTextureUV = in_textureUVs.zw;
	Out.normalUV = in_normalUVs.xy;
	Out.parentNormalUV = in_normalUVs.zw;
	
	vec3 blendedPos = mix(in_vertex0, in_vertex1, uBlendFactor).xyz;
	vec4 projPos = uModelViewProj * vec4(blendedPos, 1);
	
	vec4 cameraCoord = uModelView * vec4(blendedPos, 1);
	Out.cameraPosAndClipZ = vec4(cameraCoord.xyz, projPos.z);
	
	gl_Position = projPos;
}

