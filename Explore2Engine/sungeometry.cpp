
#include "sungeometry.h"
#include "geometries.h"

namespace explore2 {;

struct SunSurfaceVertex
{
	math::Vector3f pos;
	math::Vector2f uv;
};

void SunGeometry::static_init()
{
	using namespace glbase;

	VertexSpec::ptr vertSpec(new VertexSpec());
	vertSpec->append(VertexData::PositionData, 0, sizeof(float), 3, VertexElementType::Float);
	vertSpec->append(VertexData::TexCoord0, 1, sizeof(float), 2, VertexElementType::Float);
	assert(sizeof(SunSurfaceVertex) == vertSpec->vertexSize());

	static const int gridSize = 32;
	_sharedVerts.reset(new VertexSet(vertSpec, gridSize * gridSize * 6));
	_sharedTris.reset(new TriangleSet(glbase::TrianglePrimitiveType::TRIANGLE_STRIP));

	int vertIdx = 0, lastIdx = 0;
	Geometries::create_sphere(gridSize, 1, 
		[&](const math::Vector3f& pos) // emitted vertex
		{ 
			_sharedVerts->extract<SunSurfaceVertex>(vertIdx)->pos = pos;
			++vertIdx;
		}, 
		[&](int index) // emitted index
		{
			_sharedTris->push_back(index);
			lastIdx = index;
		},
		[&](int face) // starting new face
		{
			if(face > 0 && face < 5) // if we are imbetween two faces, create a degenerate tri in our tristrip
			{
				_sharedTris->push_back(lastIdx);
				_sharedTris->push_back(lastIdx+1);
			}
		});
	_sharedVerts->sync_all();
	_sharedTris->sync_all();

	_sharedEffect.reset(new effect::Effect());
	_sharedEffect->load("../Data/Shaders/sun_surface.xml");
}

void SunGeometry::static_release()
{
	_sharedVerts.reset();
	_sharedTris.reset();
	_sharedEffect.reset();
}

bool SunGeometry::is_visible() const
{
	return true;
}

void SunGeometry::init(	float radius, const math::Vector3f& color,
						const scene::transform::Transform::ptr& transform,
						const render::GeometrySet::ptr& geometrySet, 
						const render::GeometrySet::ptr& distantGeometrySet )
{
	_geometrySet = geometrySet;
	_radius = radius;

	scene::Material::ptr material(new scene::Material());
	material->set_effect(_sharedEffect);
	material->set_parameter("Color", color);

	scene::transform::Transform::ptr scaleTransform(new scene::transform::Transform("sun scale transform"));
	scaleTransform->set_parent(transform.get());
	scaleTransform->setTransform(math::scale<scene::transform::Transform::float_type>(radius, radius, radius));
	_geometry.reset(new scene::Geometry(_sharedTris, _sharedVerts, material, scaleTransform));
	_geometrySet->insert(_geometry);
}

void SunGeometry::update( const scene::transform::Camera::ptr& camera )
{

}

effect::Effect::ptr SunGeometry::_sharedEffect;
glbase::TriangleSet::ptr SunGeometry::_sharedTris;
glbase::VertexSet::ptr SunGeometry::_sharedVerts;

};

