#pragma once

const float C		= 1;

float calculate_depth(float z)
{
	return log(C * z + NEAR_PLANE) / log(C * FAR_PLANE + NEAR_PLANE);
}

float recover_depth(float d)
{
	return (exp(d * log(C * FAR_PLANE + NEAR_PLANE)) - NEAR_PLANE) / C;
}

const int ALBEDO_BUFFER_INDEX			= 0;
const int NORMAL_BUFFER_INDEX			= 1;
const int SPECULAR_BUFFER_INDEX			= 2;
const int POS_AND_CLIPZ_BUFFER_INDEX	= 3;

#define NO_PLANET	-1
#define NO_LIGHTING	-2

vec3 decompress_normal(in vec3 rgb)
{
	return rgb * 2 - vec3(1);
}

vec3 compress_normal(in vec3 norm)
{
	return norm * 0.5 + vec3(0.5);
}

float z_to_deferred_depth(in float z, in float near, in float far)
{
	return 1 - (z - near) / far;
}

float deferred_depth_to_z(in float depth, in float near, in float far)
{
	return near - ((depth - 1) * far);
}

vec3 project_to_window(in vec3 p, in mat4 proj, in vec4 viewport)
{
	vec4 pClip = proj * vec4(p, 1);
	vec3 pNDC = pClip.xyz / pClip.w;
	vec3 pWindow = vec3(0.5 * viewport.z * pNDC.x + viewport.x + 0.5 * viewport.z,
		0.5 * viewport.w * pNDC.y + viewport.y + 0.5 * viewport.w,
		pNDC.z);
	return pWindow;
// 			return math::Vector3d(0.5 * (double)_viewport->width() * (ppt.x / ppt.w) + (double)_viewport->left + 0.5 * (double)_viewport->width(),
// 			0.5 * (double)_viewport->height() * (ppt.y / ppt.w) + (double)_viewport->bottom + 0.5 * (double)_viewport->height(),
// 			ppt.z / ppt.w);
}

vec3 unproject_from_window(in vec3 p, in mat4 projInv, in vec4 viewport)
{
	vec4 pNDC = vec4((p.x - viewport.x - 0.5 * viewport.z) / (0.5 * viewport.z),
		(p.y - viewport.y - 0.5 * viewport.w) / (0.5 * viewport.y),
		p.z, 1.0);
	vec4 pClip = projInv * pNDC;
	if(pClip.w == 0)
		return pClip.xyz;
	return pClip.xyz / pClip.w;
// 		math::Vector4d inp((pt.x - (double)_viewport->left - 0.5 * (double)_viewport->width()) / (0.5 * (double)_viewport->width()), 
// 			(pt.y - (double)_viewport->bottom - 0.5 * (double)_viewport->height()) / (0.5 * (double)_viewport->height()), pt.z, 1.0);
// 		math::Vector4d pp((projection() * globalTransformInverse()).inverse() * Transform::vec4_type(inp));
// 		if(pp.w == 0.0)
// 			return math::Vector3d();
// 		pp.x /= pp.w;
// 		pp.y /= pp.w;
// 		pp.z /= pp.w;
}
