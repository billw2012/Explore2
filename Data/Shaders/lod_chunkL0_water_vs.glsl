


uniform mat4	uModelViewProj;
uniform mat4	uModelView;
uniform vec3	uPlanetCenterOffset;

// varying vec4	vUV;
// varying vec3	vNormal;
// varying vec4	vCameraCoordXYZandClipZ;

layout(location = 0) in vec3 in_vertex;
layout(location = 1) in vec3 uv;
layout(location = 2) in vec4 in_uv;

out FragmentIn
{
	vec3 normal;
	vec4 uv;
	vec4 cameraPosAndClipZ;
	//float clipZ;
} Out;

void main()
{
	vec4 projPos = uModelViewProj * vec4(in_vertex, 1);
	gl_Position = projPos;
	Out.normal = normalize(in_vertex + uPlanetCenterOffset);
	Out.uv = in_uv;
	vec4 cameraLocalCoord = uModelView * vec4(in_vertex, 1);
	Out.cameraPosAndClipZ = vec4(cameraLocalCoord.xyz, projPos.z);
}
