uniform mat4	uModelViewProj;

void main()
{
	gl_Position = uModelViewProj * vec4(gl_Vertex.xyz, 1);
}

