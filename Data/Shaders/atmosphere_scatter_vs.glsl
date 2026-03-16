

layout(location = 0) in vec2 in_vertex;
layout(location = 1) in vec2 in_uv;

uniform mat4	uModelViewProj;
//varying vec2	vUV;

void main()
{
	gl_Position = uModelViewProj * vec4(in_vertex, 0, 1); 
	//vUV = gl_MultiTexCoord0.xy;
}
