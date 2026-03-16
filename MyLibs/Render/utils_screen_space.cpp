#include "utils_screen_space.h"

#include "Math/frustum.hpp"
#include "Math/intersection.hpp"

namespace render {;
namespace utils {;

math::Rectanglei get_screenspace_rect( const scene::transform::Camera::ptr& camera, const math::BoundingSphered& bsphere)
{
	using namespace scene;
	using namespace math;

	static Vector3d offsets[] = 
	{
		Vector3d(-1, -1, -1),
		Vector3d( 1, -1, -1),
		Vector3d(-1,  1, -1),
		Vector3d( 1,  1, -1),
		Vector3d(-1, -1,  1),
		Vector3d( 1, -1,  1),
		Vector3d(-1,  1,  1),
		Vector3d( 1,  1,  1),
	};

	// first determine if the object intersects with the camera fustrum, if not then return a null rect
	math::Frustumd frustum = camera->get_local_frustum();
	math::BoundingSphered localBSphere(camera->localise(bsphere));
	if(!math::intersects(localBSphere, frustum).occured)
		return math::Rectanglei();

	Rectangled screenRect(std::numeric_limits<double>::max(), std::numeric_limits<double>::max(),
		 -std::numeric_limits<double>::max(), -std::numeric_limits<double>::max());

	for(size_t idx = 0; idx < 8; ++idx)
	{
		Vector3d localPos(localBSphere.center() + offsets[idx] * localBSphere.radius());
		if(localPos.z < camera->get_near_plane()) // if a point is behind the screen we have to go fullscreen for the rect
			return *(camera->get_viewport());
		Vector3d pos(camera->project_local(localPos));
		screenRect.left = std::min<double>(pos.x, screenRect.left);
		screenRect.bottom = std::min<double>(pos.y, screenRect.bottom);
		screenRect.right = std::max<double>(pos.x, screenRect.right);
		screenRect.top = std::max<double>(pos.y, screenRect.top);
	}
	return Rectanglei((int)std::floor(screenRect.left), (int)std::floor(screenRect.bottom), 
		(int)std::ceil(screenRect.right), (int)std::ceil(screenRect.top)).clamp(*(camera->get_viewport()));

	//// create the vertex spec for the mesh
	//VertexSpec::ptr fsqVSpec(new VertexSpec());
	//fsqVSpec->append(VertexData::PositionData, sizeof(float), 2, VertexElementType::Float);
	//// create the verts for the mesh
	//VertexSet::ptr fsqVerts(new VertexSet(4, fsqVSpec->vertexSize()));
	//fsqVerts->setSpec(fsqVSpec);
	//assert(sizeof(math::Vector2f) == fsqVSpec->vertexSize());
	//// top left vert
	//*fsqVerts->extract<math::Vector2f>(0) = math::Vector2f::Zero;
	//// bottom left vert
	//*fsqVerts->extract<math::Vector2f>(1) = math::Vector2f(0, screenHeight);
	//// top right
	//*fsqVerts->extract<math::Vector2f>(2) = math::Vector2f(screenWidth, 0);
	//// bottom right
	//*fsqVerts->extract<math::Vector2f>(3) = math::Vector2f(screenWidth, screenHeight);
	//// create the tri set for mesh
	//TriangleSet::ptr fsqTris(new TriangleSet(TrianglePrimitiveType::TRIANGLE_STRIP, fsqVerts, 4));
	//(*fsqTris)[0] = 0; (*fsqTris)[1] = 1; (*fsqTris)[2] = 2; (*fsqTris)[3] = 3;
	//fsqMesh->addTris(fsqTris);

	//return fsqMesh;
}

struct RectCacheKey
{
	RectCacheKey(const math::Rectanglei& rect_, UVType::type uvType_) : 
		rect(rect_), uvType(uvType_) {}

	bool operator==(const RectCacheKey& other) const 
	{
		return rect == other.rect && uvType == other.uvType;
	}
	math::Rectanglei rect;
	UVType::type uvType;
};

struct RectCacheKeyHash
{
	size_t operator()(const RectCacheKey& rect) const
	{
		return rect.rect.bottom ^ rect.rect.right ^ rect.rect.left ^ rect.rect.top ^ rect.uvType;
	}
};

struct GeometryParts
{
	glbase::TriangleSet::ptr tris;
	glbase::VertexSet::ptr verts;
};


std::unordered_map<RectCacheKey, GeometryParts, RectCacheKeyHash> rectCache;

#if 0
scene::Geometry::ptr get_shared_screen_quad( int x, int y, int width, int height, UVType::type uvType /*= UVType::Pixel*/ )
{
	using namespace scene;
	using namespace math;
	using namespace glbase;

	math::Rectanglei rect(x, y, x+width, y+height);

	auto fItr = rectCache.find(RectCacheKey(rect, uvType));
	if(fItr != rectCache.end())
	{
		return Geometry::ptr(new Geometry(fItr->second.tris, fItr->second.verts, Material::ptr(), transform::Transform::ptr(new transform::Transform())));
	}

	TriangleSet::ptr triSet(new TriangleSet(TrianglePrimitiveType::TRIANGLE_STRIP));
	triSet->push_back(0);
	triSet->push_back(1);
	triSet->push_back(2);
	triSet->push_back(3);
	triSet->sync_all();

	// create the vertex spec for the mesh
	VertexSpec::ptr vertSpec(new VertexSpec());
	vertSpec->append(VertexData::PositionData, 0, sizeof(float), 2, VertexElementType::Float);
	vertSpec->append(VertexData::TexCoord0, 1, sizeof(float), 2, VertexElementType::Float);

	// create the verts for the mesh
	VertexSet::ptr verts(new VertexSet(vertSpec, 4));

	assert(sizeof(QuadVert) == vertSpec->vertexSize());
	if(uvType == UVType::Relative)
	{
		// top left vert
		*verts->extract<QuadVert>(0) = QuadVert(math::Vector2f(x, y), math::Vector2f(0, 0));
		// bottom left vert
		*verts->extract<QuadVert>(1) = QuadVert(math::Vector2f(x, y+height), math::Vector2f(0, 1));
		// top right
		*verts->extract<QuadVert>(2) = QuadVert(math::Vector2f(x+width, y), math::Vector2f(1, 0));
		// bottom right
		*verts->extract<QuadVert>(3) = QuadVert(math::Vector2f(x+width, y+height), math::Vector2f(1, 1));
	}
	else
	{
		// top left vert
		*verts->extract<QuadVert>(0) = QuadVert(math::Vector2f(x, y), math::Vector2f(x, y));
		// bottom left vert
		*verts->extract<QuadVert>(1) = QuadVert(math::Vector2f(x, y+height), math::Vector2f(x, y+height));
		// top right
		*verts->extract<QuadVert>(2) = QuadVert(math::Vector2f(x+width, y), math::Vector2f(x+width, y));
		// bottom right
		*verts->extract<QuadVert>(3) = QuadVert(math::Vector2f(x+width, y+height), math::Vector2f(x+width, y+height));
	}
	verts->sync_all();

	GeometryParts parts;
	parts.tris = triSet;
	parts.verts = verts;
	rectCache[RectCacheKey(rect, uvType)] = parts;
	return Geometry::ptr(new Geometry(triSet, verts, scene::Material::ptr(), transform::Transform::ptr(new transform::Transform())));
}
#endif

render::GeometryRenderStage::ptr create_2D_render_stage( unsigned int width, unsigned int height, const std::string& name, FramebufferObject::ptr fbo /*= FramebufferObject::ptr()*/)
{
	using namespace scene;

	render::GeometryRenderStage::ptr fsqStage(new render::GeometryRenderStage(name));
	//fsqStage->set_included_meshes_only(true);
	fsqStage->set_fbo_target(fbo);

	transform::Camera::ptr camera(new transform::Camera());
	camera->set_type(transform::ProjectionType::Orthographic);
	camera->set_near_plane(scene::transform::Transform::float_type(-1.0));	
	camera->set_far_plane(scene::transform::Transform::float_type(1.0));
	scene::Viewport::ptr viewport(new Viewport(0, 0, width, height));
	camera->set_viewport(viewport);
	camera->set_render_area(transform::Camera::rect_type(0, 0, width, height));

	fsqStage->set_camera(camera);

	GeometrySet::ptr geometrySet(new GeometrySet());
	fsqStage->get_geometry()->add_geometry_set(geometrySet);
	return fsqStage;
}

render::GeometryRenderStage::ptr create_ui_render_stage( unsigned int width, unsigned int height, const std::string& name, FramebufferObject::ptr fbo /*= FramebufferObject::ptr()*/)
{
	using namespace scene;

	render::GeometryRenderStage::ptr fsqStage(new render::GeometryRenderStage(name));
	//fsqStage->set_included_meshes_only(true);
	fsqStage->set_fbo_target(fbo);

	transform::Camera::ptr camera(new transform::Camera());
	camera->set_type(transform::ProjectionType::Orthographic);
	camera->set_near_plane(scene::transform::Transform::float_type(-1.0));	
	camera->set_far_plane(scene::transform::Transform::float_type(1.0));
	scene::Viewport::ptr viewport(new Viewport(0, 0, width, height));
	camera->set_viewport(viewport);
	camera->set_render_area(transform::Camera::rect_type(0, height, width, 0));

	fsqStage->set_camera(camera);

	GeometrySet::ptr geometrySet(new GeometrySet());
	fsqStage->get_geometry()->add_geometry_set(geometrySet);
	return fsqStage;
}

render::GeometryRenderStage::ptr create_fsq_render_stage( unsigned int width, unsigned int height, scene::Material::ptr material, const std::string& name, FramebufferObject::ptr fbo /*= FramebufferObject::ptr()*/)
{
	using namespace scene;

	render::GeometryRenderStage::ptr fsqStage = create_2D_render_stage(width, height, name, fbo);
	GeometrySet::ptr geometrySet(fsqStage->get_geometry()->begin_geometry_sets()->geometrySet);
	scene::Geometry::ptr geom = create_new_screen_quad<unsigned int>(0, 0, width, height);
	geom->set_material(material);
	geometrySet->insert(geom);
	return fsqStage;
}

}
}