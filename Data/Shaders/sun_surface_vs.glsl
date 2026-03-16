


uniform mat4 uModelViewProj;
uniform mat4 uModelView;

layout(location = 0) in vec3 in_vertex;
layout(location = 1) in vec2 in_uv;

out FragmentIn
{
	vec2 uv;
	vec4 posAndClipz;
} Out;

void main()
{
	vec4 projPos = uModelViewProj * vec4(in_vertex, 1);
	
	Out.uv = in_uv;
	
	vec4 cameraCoord = uModelView * vec4(in_vertex, 1);
	Out.posAndClipz = vec4(cameraCoord.xyz, projPos.z);
	
	gl_Position = projPos;
}
