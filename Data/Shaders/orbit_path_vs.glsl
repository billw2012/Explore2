

uniform mat4	uModelViewProj;

layout(location = 0) in vec3 in_vertex;

void main()
{
	vec4 projPos = uModelViewProj * vec4(in_vertex, 1);
	gl_Position = projPos;
}
