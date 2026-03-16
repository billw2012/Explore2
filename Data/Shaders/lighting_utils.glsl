#pragma once

// vec3 extract_surface_lighting(in float sunAngle, in float height, 
// 	in float oneOverAtmosDepth, in sampler2D tex)
// {
// 	return texture(tex, vec2(height * oneOverAtmosDepth, 
// 		sunAngle * AngleRange)).rgb;
// }