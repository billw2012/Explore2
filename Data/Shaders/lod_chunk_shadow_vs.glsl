

uniform mat4	uModelViewProj;
uniform float	uBlendFactor;

layout(location = 0) in vec3 in_vertex1;
layout(location = 1) in vec3 in_vertex0;

void main()
{
	vec3 blendedPos = mix(in_vertex0, in_vertex1, uBlendFactor).xyz;
	gl_Position = uModelViewProj * vec4(blendedPos, 1);
}

