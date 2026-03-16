uniform mat4 uModelViewMatrix;

void main()
{
	gl_Position = uModelViewMatrix * gl_Vertex;
}
