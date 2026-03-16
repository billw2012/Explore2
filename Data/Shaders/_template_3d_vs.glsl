

uniform mat4	uModelViewProj;
uniform mat4	uModelView;

layout(location = 0) in vec3 in_vertex1;
layout(location = 1) in vec3 in_vertex0;
layout(location = 2) in vec4 in_textureUVs;
layout(location = 3) in vec4 in_normalUVs;

out FragmentIn
{
	vec4 cameraPosAndClipZ;
} Out;


void main()
{
	vec4 projPos = uModelViewProj * vec4(blendedPos, 1);
	vec4 cameraCoord = uModelView * vec4(blendedPos, 1);
	Out.cameraPosAndClipZ = vec4(cameraCoord.xyz, projPos.z);	
	gl_Position = projPos;
}

