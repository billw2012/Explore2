
#include "geometries.h"
#include "Math/matrix3.hpp"
#include "Math/matrix4.hpp"
#include "Math/transformation.hpp"
#include "Math/vector4.hpp"
#include <vector>
#include <cassert>

namespace explore2 {;

using namespace math;

namespace {;

void create_edge(int ptCount, const Vector3f& startpt, const Vector3f& endpt, std::vector< Vector3f >& verts)
{
	using namespace math;
	Vector3f raxis = startpt.crossp(endpt).normal();
	float fullangle = math::rad_to_deg(startpt.normal().angle(endpt.normal()));

	float ad = fullangle / (ptCount-1);
	// create intermediate pts
	for(int idx = 0; idx < ptCount; ++idx)
	{
		float angle = ad * idx;
		Matrix4f rmat = math::rotate_axis_angle(raxis, angle);
		verts.push_back(Vector3f(rmat * (Vector4f(startpt) + Vector4f::WAxis)));
	}
	// set end pt index
}

void create_face(const std::vector< Vector3f >& left, bool revL, 
				const std::vector< Vector3f >& right, bool revR,
				std::function<void (const math::Vector3f&)> emitVertex)
{
	using namespace math;
	using namespace std;

	// assert the dimensions are correct
	assert(left.size() == right.size());

	size_t gridSize = left.size();

	Vector3f tlpt = left[0];
	Vector3f blpt = left[gridSize-1];
	float yfullangle = rad_to_deg(tlpt.normal().angle(blpt.normal()));

	for(size_t y = 0; y < gridSize; ++y)
	{
		Vector3f startpt = revL ? left[gridSize - 1 - y] : left[y];
		Vector3f endpt = revR ? right[gridSize - 1 - y] : right[y];
		Vector3f raxis = startpt.crossp(endpt).normal();
		float xfullangle = rad_to_deg(startpt.normal().angle(endpt.normal()));

		float dxangle = xfullangle / (gridSize-1);
		float xangle = 0;
		for(size_t x = 0; x < gridSize; ++x, xangle += dxangle)
		{
			Matrix4f rmat = math::rotate_axis_angle(raxis, xangle);
			emitVertex(Vector3f(rmat * (Vector4f(startpt) + Vector4f::WAxis)));
		}
	}
}

void build_tri_strip(int gridSize, int ptIdxOffset, std::function<void (int)> emitIndex)
{
	bool skipping = true, started = false;

	int lastidx = 0;
	for(int y = 0, idx = 0; y < gridSize - 1; ++y)
	{
		if(y > 0 && y < gridSize - 2)
		{
			// create degenerate triangle
			emitIndex(lastidx);
			emitIndex(idx + ptIdxOffset);
		}

		for(int x = 0; x < gridSize; ++x, ++idx)
		{ 
			emitIndex(idx + ptIdxOffset);
			lastidx = idx + gridSize + ptIdxOffset;
			emitIndex(lastidx);
		}
		//if(y != gridSize - 2)
		//	// create degenerate triangle
		//	emitIndex(lastidx);
	}
}

}


void Geometries::create_sphere(int gridSize, float radius, std::function<void (const math::Vector3f&)> emitVertex, std::function<void (int)> emitIndex,
							   std::function<void (int)> emitFace)
{
	Vector3f corners[8];

	float size = radius * (1/Vector3f::One.length());
	corners[0] = Vector3f(-size,  size,  size); // l t n
	corners[1] = Vector3f( size,  size,  size); // r t n
	corners[2] = Vector3f(-size, -size,  size); // l b n
	corners[3] = Vector3f( size, -size,  size); // r b n
	corners[4] = Vector3f( size,  size, -size); // r t f
	corners[5] = Vector3f(-size,  size, -size); // l t f
	corners[6] = Vector3f( size, -size, -size); // r b f
	corners[7] = Vector3f(-size, -size, -size); // l b f

	std::vector< Vector3f > edges[8];
	// create gridSized sets of vertices separated equally by angle
	create_edge((int)gridSize, corners[0], corners[2], edges[0]); // left front
	create_edge((int)gridSize, corners[1], corners[3], edges[1]); // right front
	create_edge((int)gridSize, corners[4], corners[6], edges[2]); // right back
	create_edge((int)gridSize, corners[5], corners[7], edges[3]); // left back
	create_edge((int)gridSize, corners[5], corners[0], edges[4]); // left top
	create_edge((int)gridSize, corners[7], corners[2], edges[5]); // left bottom
	create_edge((int)gridSize, corners[1], corners[4], edges[6]); // right top
	create_edge((int)gridSize, corners[3], corners[6], edges[7]); // right bottom

	create_face(edges[0], false, edges[1], false, emitVertex); // front
	create_face(edges[2], false, edges[3], false, emitVertex); // back
	create_face(edges[3], false, edges[0], false, emitVertex); // left
	create_face(edges[1], false, edges[2], false, emitVertex); // right
	create_face(edges[4], false, edges[6], true,  emitVertex); // top
	create_face(edges[5], true,  edges[7], false, emitVertex); // bottom

	for(int idx = 0; idx < 6; ++idx)
	{
		emitFace(idx);
		build_tri_strip(gridSize, idx * gridSize * gridSize, emitIndex);
	}
}

}