uniform mat4	uModelViewProj;
uniform mat4	uModelView;

//varying vec3	vTextureUV;
//varying vec3	vNormalUV;
//varying vec4	vCameraCoordXYZandClipZ;
layout(location = 0) in vec3 in_vertex;
layout(location = 1) in vec3 in_textureUV;
layout(location = 2) in vec3 in_normalUV;

out FragmentIn
{
	vec3 textureUV;
	vec3 normalUV;
	vec4 cameraPosAndClipZ;
} Out;

void main()
{
	Out.textureUV = in_textureUV;
	Out.normalUV = in_normalUV;
	
	vec4 projPos = uModelViewProj * vec4(in_vertex, 1);
	
	vec4 cameraCoord = uModelView * vec4(in_vertex, 1);
	Out.cameraPosAndClipZ = vec4(cameraCoord.xyz, projPos.z);
	
	gl_Position = projPos;
}

