#ifndef sungeometry_h__
#define sungeometry_h__

#include <memory>
#include "Effect/effect.h"
#include "GLbase/vertexset.hpp"
#include "GLbase/triangleset.hpp"
#include "Scene/transform.hpp"
#include "Scene/camera.hpp"
#include "Scene/geometry.hpp"
#include "Render/geometryset.h"


namespace explore2 {;

struct SunGeometry
{
	typedef std::shared_ptr<SunGeometry> ptr;

	static void static_init();
	static void static_release();

	bool is_visible() const;

	void init(float radius, const math::Vector3f& color,
		const scene::transform::Transform::ptr& transform,
		const render::GeometrySet::ptr& geometrySet, 
		const render::GeometrySet::ptr& distantGeometrySet);

	void update(const scene::transform::Camera::ptr& camera);

	float get_radius() const { return _radius; }

private:
	float _radius;

	scene::Geometry::ptr _geometry;
	render::GeometrySet::ptr _geometrySet;

	static glbase::VertexSet::ptr _sharedVerts;
	static glbase::TriangleSet::ptr _sharedTris;
	static effect::Effect::ptr _sharedEffect;
};

}

#endif // sungeometry_h__
