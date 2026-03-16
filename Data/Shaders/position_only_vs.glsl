


uniform mat4	uModelViewProj;

layout(location = 0) in vec3 in_vertex;


void main()
{
	gl_Position = uModelViewProj * vec4(in_vertex, 1);
}
