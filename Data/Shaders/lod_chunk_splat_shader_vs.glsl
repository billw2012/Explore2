


uniform mat4 uModelViewProj;

layout(location = 0) in vec2 in_vertex;
layout(location = 1) in vec3 in_uv;
//varying vec3 vUV;
out FragmentIn
{
	vec3 uv;
} Out;

void main()
{
	gl_Position = uModelViewProj * vec4(in_vertex, 0, 1); 
	Out.uv = in_uv;
}
