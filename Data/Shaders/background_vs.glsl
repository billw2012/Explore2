

uniform mat4	uModelViewProj;
uniform mat4	uModelView;

layout(location = 0) in vec3 in_vertex;

out FragmentIn
{
	vec4 posAndClipz;
} Out;

void main()
{
	vec4 projPos = uModelViewProj * vec4(in_vertex, 1);
	vec4 cameraCoord = uModelView * vec4(in_vertex, 1);

	Out.posAndClipz = vec4(cameraCoord.xyz, projPos.z);
	
	gl_Position = projPos;
}
