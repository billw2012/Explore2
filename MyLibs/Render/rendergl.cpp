// standard
#include <limits>
#include <functional>

// solution
#include "Math/transformation.hpp"
#include "Math/highprec.h"
#include "Remote/RemoteDebug.h"
#include "Vice/renderer.h"

// project
#include "rendergl.h"
#include "utils_screen_space.h"

/*

new effect shader architecture:

- shader code in .cg files like usual.
- effects are xml files of this format:

<effect name = "effect_name">
	<vert_program>
		<file name = "dir\source_file.cg" func = "vp_func_name"/>
		<map_param source = "ModelViewMatrix" target = "mat" />
	</vert_program>
	<frag_program>
		<file name = "dir\source_file.cg" func = "fp_func_name"/>
		<map_param source = "ColourTexture" target = "colTex" />
	</frag_program>
</effect>

*/

using namespace glbase;
using namespace scene::transform;

namespace render {;

namespace {;

template < class PTy > 
bool operator < (const std::shared_ptr< PTy >& left, const std::shared_ptr< PTy >& right) 
{
	return left.get() < right.get();
}

template < class PTy > 
bool operator > (const std::shared_ptr< PTy >& left, const std::shared_ptr< PTy >& right) 
{
	return left.get() > right.get();
}

struct DepthRange
{
	DepthRange(Transform::float_type neard_ = Transform::float_type(std::numeric_limits<double>::max()), 
		Transform::float_type fard_ = Transform::float_type(-std::numeric_limits<double>::max())) : neard(neard_), fard(fard_) {}
	Transform::float_type neard, fard;
};

struct GeometryWithRange
{
	GeometryWithRange(const scene::Geometry::ptr& geom = scene::Geometry::ptr(),
		float distance_ = 0.0f, const DepthRange& range_ = DepthRange(),
		const Transform::bsph_type& globalBS_ = Transform::bsph_type()) 
		: geometry(geom), distance(distance_), range(range_), globalBS(globalBS_) {}

	scene::Geometry::ptr geometry;
	float distance;
	DepthRange range;
	Transform::bsph_type globalBS;

	bool operator < (const GeometryWithRange& right) const
	{
		return geometry < right.geometry;
	}
};

struct GeometryWithDistanceLessThanFTB
{
	bool operator()(const GeometryWithRange& left,
		const GeometryWithRange& right) const
	{
		if(left.geometry->get_material()->get_effect() < right.geometry->get_material()->get_effect())
			return true;
		if(left.geometry->get_material()->get_effect() > right.geometry->get_material()->get_effect())
			return false;

		if(left.geometry->get_material() < right.geometry->get_material())
			return true;
		if(left.geometry->get_material() > right.geometry->get_material())
			return false;

		if(left.geometry->get_transform() < right.geometry->get_transform())
			return true;
		if(left.geometry->get_transform() > right.geometry->get_transform())
			return false;

		if(left.geometry->get_verts() < right.geometry->get_verts())
			return true;
		if(left.geometry->get_verts() > right.geometry->get_verts())
			return false;

		if(left.geometry->get_tris() < right.geometry->get_tris())
			return true;
		if(left.geometry->get_tris() > right.geometry->get_tris())
			return false;

		if(left.distance < right.distance)
			return true;
		if(left.distance > right.distance)
			return false;

		return left.geometry < right.geometry;
	}
};

struct GeometryWithDistanceLessThanBTF
{
	bool operator()(const GeometryWithRange& left,
		const GeometryWithRange& right) const
	{
		if(left.geometry->get_material()->get_effect() < right.geometry->get_material()->get_effect())
			return true;
		if(left.geometry->get_material()->get_effect() > right.geometry->get_material()->get_effect())
			return false;

		if(left.geometry->get_material() < right.geometry->get_material())
			return true;
		if(left.geometry->get_material() > right.geometry->get_material())
			return false;

		if(left.geometry->get_transform() < right.geometry->get_transform())
			return true;
		if(left.geometry->get_transform() > right.geometry->get_transform())
			return false;

		if(left.geometry->get_verts() < right.geometry->get_verts())
			return true;
		if(left.geometry->get_verts() > right.geometry->get_verts())
			return false;

		if(left.geometry->get_tris() < right.geometry->get_tris())
			return true;
		if(left.geometry->get_tris() > right.geometry->get_tris())
			return false;

		if(left.distance > right.distance)
			return true;
		if(left.distance < right.distance)
			return false;

		return left.geometry < right.geometry;
	}
};

typedef std::set< GeometryWithRange, GeometryWithDistanceLessThanFTB > 
	SortedGeometryFTBSet;
typedef std::set< GeometryWithRange, GeometryWithDistanceLessThanBTF > 
	SortedGeometryBTFSet;
typedef std::set< GeometryWithRange > UnsortedGeometrySet;

struct GeometryWithDistanceLessThanFTBDepth
{
	bool operator()(const GeometryWithRange& left,
		const GeometryWithRange& right) const
	{
		if(left.distance < right.distance)
			return true;
		if(left.distance > right.distance)
			return false;

		return left.geometry < right.geometry;
	}
};

typedef std::set< GeometryWithRange, GeometryWithDistanceLessThanFTBDepth > 
	SortedGeometryFTBDepthSet;

struct GeometrySetKey
{
	GeometrySetKey() {}

	GeometrySetKey(ComposedGeometrySet::ptr sourceGeometrySet_, scene::transform::Camera::ptr camera_)
		: sourceGeometrySet(sourceGeometrySet_),
		camera(camera_)
	{}

	bool operator<(const GeometrySetKey& other) const
	{
		if(sourceGeometrySet < other.sourceGeometrySet)
			return true;
		if(sourceGeometrySet > other.sourceGeometrySet)
			return false;
		return camera < other.camera;
	}

	ComposedGeometrySet::ptr sourceGeometrySet;
	scene::transform::Camera::ptr camera;
};

struct SortedGeometrySet
{
	DepthRange range;
	SortedGeometryFTBSet geom;
};

struct CachedGeometrySet
{
	SortedGeometrySet fullSet;
	std::unordered_map<scene::transform::Camera::ptr, SortedGeometrySet> culledSets;
};

typedef std::unordered_map< ComposedGeometrySet::ptr, CachedGeometrySet > CachedGeometrySetMap;

//VideoMemoryManager::ptr _vmm;
FramebufferObject::ptr _boundFBO;

// diagnostic vars
unsigned int _shaderBinds;

SemanticParamSetMap _semanticMatrixSetMap;

bool _depthSplitEnabled;

CachedGeometrySetMap _geometrySets;

scene::Material::ptr _drawTextureMat;

scene::Geometry::ptr _sphere;

scene::Geometry::ptr _fontGeometry;

const int FONT_MAX_CHARS = 1024;

struct FontVertex
{
	math::Vector2f pos;
	math::Vector2f uv;
};

template < class RenderStageVectorTy_, class RSItr_ >
void insert_stages(RenderStageVectorTy_& stages, RSItr_ start, RSItr_ end);

void render_geometry_render_stage(GeometryRenderStage::ptr rs, const SceneContext& sc);
void render_lighting_render_stage(LightingRenderStage::ptr rs, const SceneContext& sc);
void render_atmospheric_render_stage(AtmosphericsRenderStage::ptr rs, const SceneContext& sc);

void render_solar_light(LightingRenderStage::ptr rs, LightingRenderStage::LightParams::ptr lightParams);

struct LightFustrum
{
	LightFustrum(scene::transform::Transform::float_type fov_, scene::transform::Transform::float_type ratio_) : fov(fov_), ratio(ratio_) {}
	LightFustrum() {}

	scene::transform::Transform::float_type fov, fard, neard, ratio;
	scene::transform::Transform::vec3_type point[8];
};

static void update_split_dist(std::vector<LightFustrum>& f, scene::transform::Transform::float_type nd, scene::transform::Transform::float_type fd);
static scene::transform::Camera::ptr applyCropMatrix(const math::Vector3f& lightDir, const SortedGeometrySet& geometrySet, LightFustrum &f, SortedGeometryFTBDepthSet& lightCulledSet, float maxOccluderRange);

static void updateFrustumPoints(LightFustrum &f, const scene::transform::Transform::vec3_type &center, const scene::transform::Transform::vec3_type &cameraForward, const scene::transform::Transform::vec3_type& cameraUp, const scene::transform::Transform::vec3_type& cameraRight);
// 	void draw_geometries(const UnsortedGeometrySet &meshSet, 
// 		const math::Planef& globalCameraPlane,
// 		scene::transform::Camera::ptr camera, RenderStage::ptr rs);

void apply_symantic_params(effect::Effect::ptr effect, 
						   scene::transform::Transform::ptr trans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode);

void draw_geometry(const scene::Geometry::ptr& geometry, const scene::transform::Camera::ptr& camera, effect::Effect::EffectMode::type mode = effect::Effect::EffectMode::Render);

void create_basic_geoms();

template < class RenderStageVectorTy_, class RSItr_ >
void insert_stages(RenderStageVectorTy_& stages, RSItr_ start, RSItr_ end)
{
	for(RSItr_ sItr = start; sItr != end; ++sItr)
	{
		RenderStageVectorTy_::iterator findItr = std::find(stages.begin(), stages.end(), *sItr);
		if(findItr == stages.end())
		{
			insert_stages(stages, (*sItr)->begin_dependancies(), (*sItr)->end_dependancies());
			stages.push_back(*sItr);
		}
	}
}

template < class ItrTy_ > 
void draw_depth_range(ItrTy_ begin, ItrTy_ end, const scene::transform::Camera::ptr& camera, effect::Effect::EffectMode::type effectMode = effect::Effect::EffectMode::Render)
{
	for(auto itr = begin; itr != end; ++itr)
	{
		draw_geometry(itr->geometry, camera, effectMode);
	}
}

/*

To render:

- cull by full frustum
- split between near and far
- split near and far between trans and not trans
- sort trans from far to near, sort not trans from near to far
- render each set

*/

// RenderGL() 
// 	: _nearMaxExtents(Transform::float_type(1.0), Transform::float_type(std::numeric_limits<float>::max())),
// 	_farMaxExtents(Transform::float_type(100.0), Transform::float_type(std::numeric_limits<float>::max()))
// {
// // 	_drawTextureMat = scene::Material(new scene::Material());
// // 	effect::Effect::ptr eff(new effect::Effect());
// // 	if(!eff->load("../Data/Shaders/draw_single_texture.xml"))
// // 	{
// // 		std::cout << "Error: " << eff->get_last_error() << std::endl;
// // 		return 1;
// // 	}
// // 	_drawTextureMat->set_effect(eff);
// }

// void setVMM( VideoMemoryManager::ptr vmm )
// {
// 	_vmm = vmm;
// }

//void add_geometry_to_set_with_exclusion(const scene::Geometry::ptr& geometry, 
//	UnsortedGeometrySet& geometrySet, RenderStage::ptr rs)
//{
//	if(geometry->is_valid() && !rs->excludes_geometry(geometry))
//		geometrySet.insert(geometry);
//}
//
//void add_geometry_to_set_without_exclusion(const scene::Geometry::ptr& geometry, 
//	UnsortedGeometrySet& geometrySet)
//{
//	if(geometry->is_valid())
//		geometrySet.insert(geometry);
//}

const SortedGeometrySet& get_geometry_set(const ComposedGeometrySet::ptr& compGeomSet, const scene::transform::Camera::ptr& camera, bool cull)
{
	math::Plane<Transform::float_type> globalCameraPlane((camera->forwardGlobal()), 
		(camera->centerGlobal()));

	auto fItr = _geometrySets.find(compGeomSet);
	if(fItr == _geometrySets.end())
	{
		fItr = _geometrySets.insert(std::make_pair(compGeomSet, CachedGeometrySet())).first;
		CachedGeometrySet& fSet = fItr->second;
		for(auto gsItr = compGeomSet->begin_geometry_sets(); gsItr != compGeomSet->end_geometry_sets(); ++gsItr)
		{
			ComposedGeometrySet::GeometrySetAndOp& geomSetAndOp = *gsItr;
			if(geomSetAndOp.op == ComposedGeometrySet::GeometrySetOp::Include)
			{
				for(GeometryIterator gItr = geomSetAndOp.geometrySet->begin(); 
					gItr != geomSetAndOp.geometrySet->end(); ++gItr)
				{
					const scene::Geometry::ptr& geom = *gItr;
					Transform::bsph_type globalBS(geom->get_transform()->get_global_bsphere(Transform::aabb_type(geom->get_aabb())));
					float distance = (float)globalCameraPlane.distance(globalBS.center());
					fSet.fullSet.geom.insert(GeometryWithRange(geom, distance, DepthRange(distance - globalBS.radius(), distance + globalBS.radius()), globalBS));
				}
			}
			else if(geomSetAndOp.op == ComposedGeometrySet::GeometrySetOp::Exclude)
			{
				for(GeometryIterator gItr = geomSetAndOp.geometrySet->begin(); 
					gItr != geomSetAndOp.geometrySet->end(); ++gItr)
				{
					fSet.fullSet.geom.erase(*gItr);
				}
			}
		}

		Transform::float_type neard = std::numeric_limits<Transform::float_type>::max(), fard = -std::numeric_limits<Transform::float_type>::max();
		for(auto itr = fSet.fullSet.geom.begin(); itr != fSet.fullSet.geom.end(); ++itr)
		{
			neard = std::min(neard, itr->range.neard);
			fard = std::max(fard, itr->range.fard);
		}
		fSet.fullSet.range = DepthRange(std::max(camera->get_near_plane(), neard), std::min(camera->get_far_plane(), fard));
	}
	CachedGeometrySet& fSet = fItr->second;

	if(!cull)
		return fSet.fullSet;

	auto fItr2 = fSet.culledSets.find(camera);
	if(fItr2 != fSet.culledSets.end())
		return fItr2->second;
	
	SortedGeometrySet& cameraSet = fSet.culledSets[camera];

	math::Frustumd frustum = camera->get_global_frustum();

	Transform::float_type neard = std::numeric_limits<Transform::float_type>::max(), fard = -std::numeric_limits<Transform::float_type>::max();
	for(auto gItr = fSet.fullSet.geom.begin(); gItr != fSet.fullSet.geom.end(); ++gItr)
	{
		const GeometryWithRange& geom = *gItr;
		if(math::intersects(geom.globalBS, frustum).occured)
		{
			cameraSet.geom.insert(geom);
			neard = std::min(neard, geom.range.neard);
			fard = std::max(fard, geom.range.fard);
		}		
	}
	cameraSet.range = DepthRange(std::max(camera->get_near_plane(), neard), std::min(camera->get_far_plane(), fard));

	return cameraSet;
}

static Transform::float_type gSplitWeight(0.75);

// updateSplitDist computes the near and far distances for every frustum slice
// in camera eye space - that is, at what distance does a slice start and end
void update_split_dist(std::vector<LightFustrum>& f, Transform::float_type nd, Transform::float_type fd)
{
	Transform::float_type lambda = gSplitWeight;
	Transform::float_type ratio = fd/nd;
	f[0].neard = nd;

	for(int i=1; i<f.size(); i++)
	{
		Transform::float_type si = Transform::float_type(i) / (Transform::float_type)f.size();

		f[i].neard = lambda*(nd*pow(ratio, si)) + (Transform::float_type(1)-lambda)*(nd + (fd - nd)*si);
		f[i-1].fard = f[i].neard * Transform::float_type(1.005);
	}
	f[f.size()-1].fard = fd;
}

enum BoxPointIndices {
	FrontTopLeft		= 0,
	FrontBottomLeft		= 1,
	FrontBottomRight	= 2,
	FrontTopRight		= 3,
	BackTopLeft			= 4,
	BackBottomLeft		= 5,
	BackBottomRight		= 6,
	BackTopRight		= 7
};

// Compute the 8 corner points of the current view frustum
void updateFrustumPoints(LightFustrum &f, const Transform::vec3_type &center, const Transform::vec3_type &cameraForward, const Transform::vec3_type& cameraUp, const Transform::vec3_type& cameraRight)
{
	//Vector3f up(0.0f, 1.0f, 0.0f);
	//Vector3f right = view_dir.crossp(up);

	Transform::vec3_type fc = center + cameraForward*f.fard;
	Transform::vec3_type nc = center + cameraForward*f.neard;

	//right = right.normal();
	//up = right.crossp(view_dir).normal();

	// these heights and widths are half the heights and widths of
	// the near and far plane rectangles
	Transform::float_type near_height = tan(f.fov/Transform::float_type(2.0)) * f.neard;
	Transform::float_type near_width = near_height * f.ratio;
	Transform::float_type far_height = tan(f.fov/Transform::float_type(2.0)) * f.fard;
	Transform::float_type far_width = far_height * f.ratio;

	// front top left
	f.point[FrontTopLeft] = nc + cameraUp * near_height - cameraRight * near_width;
	// front bottom left
	f.point[FrontBottomLeft] = nc - cameraUp * near_height - cameraRight * near_width;
	// front bottom right
	f.point[FrontBottomRight] = nc - cameraUp * near_height + cameraRight * near_width;
	// front top right
	f.point[FrontTopRight] = nc + cameraUp * near_height + cameraRight * near_width;

	// back top left
	f.point[BackTopLeft] = fc + cameraUp * far_height - cameraRight * far_width;
	// back bottom left
	f.point[BackBottomLeft] = fc - cameraUp * far_height - cameraRight * far_width;
	// back bottom right
	f.point[BackBottomRight] = fc - cameraUp * far_height + cameraRight * far_width;
	// back top right
	f.point[BackTopRight] = fc + cameraUp * far_height + cameraRight * far_width;
}

// this function builds a projection matrix for rendering from the shadow's POV.
// First, it computes the appropriate z-range and sets an orthogonal projection.
// Then, it translates and scales it, so that it exactly captures the bounding box
// of the current frustum slice
scene::transform::Camera::ptr applyCropMatrix(const math::Vector3f& lightDir, const SortedGeometrySet& geometrySet, LightFustrum &f, SortedGeometryFTBDepthSet& lightCulledSet, float maxOccluderRange)
{
	using namespace math;
	using namespace scene::transform;

	Transform::float_type maxX(-std::numeric_limits<double>::max());
	Transform::float_type maxY(-std::numeric_limits<double>::max());
	Transform::float_type maxZ(-std::numeric_limits<double>::max());
	Transform::float_type minX(std::numeric_limits<double>::max());
	Transform::float_type minY(std::numeric_limits<double>::max());
	Transform::float_type minZ(std::numeric_limits<double>::max());

	Transform::matrix4_type lightDirMatrix(math::look_at(Vector3d::Zero, Vector3d(lightDir), Vector3d::XAxis));
	Transform::matrix4_type lightMatInv(lightDirMatrix.inverse());
	Transform::vec4_type transf;	

	// first determine in xyz extents of the camera frustum slice in the light local
	// coordinates
	for(int i=0; i<8; i++)
	{
		transf = Transform::vec4_type(lightMatInv * Transform::vec4_type(f.point[i], 1.0));
 		if(transf.x > maxX) maxX = transf.x;
 		if(transf.x < minX) minX = transf.x;
 		if(transf.y > maxY) maxY = transf.y;
 		if(transf.y < minY) minY = transf.y;
		if(transf.z > maxZ) maxZ = transf.z;
		if(transf.z < minZ) minZ = transf.z;
	}

	maxZ += maxOccluderRange;
	// we don't want to cull out any objects by minimum z range, only maximum
	AABBd lightLocalAABB(Vector3d(minX, minY, minZ), Vector3d(maxX, maxY, maxZ));

	// generate the set of objects that can shadow the camera frustum slice (might include objects outside the camera frustum itself!)
 	for(auto gItr = geometrySet.geom.begin(); gItr != geometrySet.geom.end(); ++gItr)
 	{
 		const GeometryWithRange& geom = *gItr;
 
 		if(geom.geometry->is_flag_set(scene::Geometry::Flags::CastShadows))
 		{
 			BoundingSphered cameraLocalBS(
				Vector3d((lightMatInv*geom.geometry->get_transform()->globalTransform()) * Transform::vec4_type(geom.geometry->get_bsphere().center(), 1.0f)),
 				geom.geometry->get_bsphere().radius());
 
 			if(intersects(cameraLocalBS, lightLocalAABB).occured)
 			{
 				double closePt = cameraLocalBS.center().z + cameraLocalBS.radius();
 				//if(closePt > maxZ)
				//	maxZ = closePt;
				lightCulledSet.insert(GeometryWithRange(geom.geometry, (float)closePt));
 			}		
 		}
 	}

	// now we have the light local aabb of all objects that need to rendered we can
	// determine the actual light camera parameters:
	Transform::float_type width = (maxX - minX),
	 		height = (maxY - minY),
	 		depth = (maxZ - minZ);

	Transform::float_type offs(10);
	lightDirMatrix = math::translate(
		Transform::vec3_type(lightDirMatrix.getColumnVector(0)) * Transform::float_type(minX) + 
		Transform::vec3_type(lightDirMatrix.getColumnVector(1)) * Transform::float_type(minY) + 
		Transform::vec3_type(lightDirMatrix.getColumnVector(2)) * Transform::float_type(maxZ + offs)) * lightDirMatrix;
	Camera::ptr lightCamera(new scene::transform::Camera());
	lightCamera->setTransform(lightDirMatrix);
	lightCamera->set_type(ProjectionType::Orthographic);
	lightCamera->set_near_plane(Camera::float_type(offs));
 	lightCamera->set_far_plane(Camera::float_type(depth + offs));
	lightCamera->set_render_area(Camera::rect_type(Transform::float_type(0), Transform::float_type(0), width, height));
	return lightCamera;
}

using namespace scene::transform;

std::string serialize_matrix(const math::Matrix4f& mat)
{
	std::stringstream ss;
	ss << mat.m11 << ", " << mat.m12 << ", " << mat.m13 << ", " << mat.m14 << ", ";
	ss << mat.m21 << ", " << mat.m22 << ", " << mat.m23 << ", " << mat.m24 << ", ";
	ss << mat.m31 << ", " << mat.m32 << ", " << mat.m33 << ", " << mat.m34 << ", ";
	ss << mat.m41 << ", " << mat.m42 << ", " << mat.m43 << ", " << mat.m44;
	return ss.str();
}

void render_solar_light(LightingRenderStage::ptr rs, LightingRenderStage::LightParams::ptr lightParams)
{
	using namespace scene;
	using namespace math;

	// NOTE:
	// To apply planet shadows draw planet spheres unclipped with clamping to far plane.
	// Objects in space? 
	// Could be rendered only using the lighting for a single planet (the closest one).
	// Renderer doesn't know about planets :/
	// Could use alpha component to determine how lighting is applied to objects outside the atmosphere.
	// Could assign objects in space to planets.
	// Transition? Could fade objects in space between lighting conditions of all planets being rendered...
	// No, individual light passes only know about one light source and one planet center.
	// Generalize:
	// Solar light defined both atmospheric conditions and light source.
	// If there are two planets (A, B) and two suns (S, T) then there are potentially 6 solar lights:
	// S->A, T->A, B->A, S->B, T->B, A->B.
	// One solar light can only light one planet.
	// Therefore some how geometry must be clipped from being drawn for a light it does not belong to.
	// 
	// Above renderer assign all geometry to solar lights explicitly based on heuristic.
	// 
	// e.g. Two planets: one large (A), one small in orbit (B), one sun (S).
	// A is lit by S only.
	// B is lit by A and S.
	// Draw S->A first, 

	// get the view camera
	scene::transform::Camera::ptr mainCamera = rs->get_geometry_stage()->get_camera();
	// prepare the light fustrums
	std::vector<LightFustrum> lightFrustums(rs->get_cascade_splits(), 
		LightFustrum(Transform::float_type(math::deg_to_rad(mainCamera->getFOV())), 
		Transform::float_type(mainCamera->get_viewport()->width()) / Transform::float_type(mainCamera->get_viewport()->height())));

	Transform::vec3_type cameraPos(mainCamera->centerGlobal());
	Transform::vec3_type cameraForward(mainCamera->forwardGlobal());
	Transform::vec3_type cameraUp(mainCamera->upGlobal());
	Transform::vec3_type cameraRight(mainCamera->rightGlobal());

	// get the full geometry set
	const SortedGeometrySet& geometrySet = get_geometry_set(rs->get_geometry_stage()->get_geometry(), mainCamera, false);

	// if there are no opaque objects then return
	//if(geometrySet.opaqueSet.empty())
	//	return;

	scene::transform::Light::ptr light = lightParams->get_light();
	Transform::float_type limitedMaxFar = std::min(geometrySet.range.fard, Transform::float_type(light->get_max_shadow_distance()));
	Transform::float_type limitedMaxNear = std::min(geometrySet.range.neard, Transform::float_type(light->get_max_shadow_distance()));
	//if(limitedMaxFar <= geometrySet.fullRange.neard)
	//	return;

	// update the split distances in the light fustrums
	update_split_dist(lightFrustums, limitedMaxNear, limitedMaxFar);

	rs->get_shadow_fbo()->BindDraw();
	rs->get_shadow_fbo()->BindDrawBuffers();
	//GLenum attachment[] = {GL_COLOR_ATTACHMENT0}; 

	GLint shadowWidth = rs->get_cascade_shadow_depth_texture()->width();
	GLint shadowHeight = rs->get_cascade_shadow_depth_texture()->height();
	glViewport(0, 0, shadowWidth, shadowHeight);

	GLboolean polygonOffsetFillEnabled = glIsEnabled(GL_POLYGON_OFFSET_FILL);
	GLfloat oldPolygonOffsetFactor, oldPolygonOffsetUnits;
	glGetFloatv(GL_POLYGON_OFFSET_FACTOR, &oldPolygonOffsetFactor);
	glGetFloatv(GL_POLYGON_OFFSET_UNITS, &oldPolygonOffsetUnits);
	glPolygonOffset(10.0f, 4096.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);
	
	// this matrix does the conversion from normalized device coordinates to texture and depth coordinates
	const Transform::matrix4_type biasMatrix(
		0.5, 0.0, 0.0, 0.5, 
		0.0, 0.5, 0.0, 0.5,
		0.0, 0.0, 0.5, 0.5,
		0.0, 0.0, 0.0, 1.0);

	std::vector<float> farDistances(lightFrustums.size());
	std::vector<math::Matrix4f> lightFrustumMatrices(lightFrustums.size());

	math::Vector3f lightDir((cameraPos - light->centerGlobal()).normal());

	Transform::matrix4_type cameraViewMatrix(mainCamera->globalTransform());
	// generate the shadow maps
	for(size_t idx = 0; idx < lightFrustums.size(); ++idx)
	{
		// compute the camera frustum slice boundary points in world space
		updateFrustumPoints(lightFrustums[idx], cameraPos, cameraForward, cameraUp, cameraRight);
		// adjust the view frustum of the light, so that it encloses the camera frustum slice fully.
		// note that this function sets the projection matrix as it sees best fit
		SortedGeometryFTBDepthSet lightCulledSet;
		scene::transform::Camera::ptr lightCamera = applyCropMatrix(-lightDir, geometrySet, lightFrustums[idx], lightCulledSet, light->get_max_shadow_occluder_distance());

		// setup the get_viewport
		glFramebufferTextureLayer(GL_DRAW_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, rs->get_cascade_shadow_depth_texture()->handle(), 0, static_cast<GLint>(idx));

		glClear(GL_DEPTH_BUFFER_BIT);

		draw_depth_range(lightCulledSet.begin(), lightCulledSet.end(), lightCamera, effect::Effect::EffectMode::Shadow);

		// project the far distance
		Transform::vec4_type fvec(Transform::float_type(0.0), Transform::float_type(0.0), -lightFrustums[idx].fard, Transform::float_type(1.0));
		fvec = mainCamera->projection() * fvec;
		// project the far distance to clip space.
		farDistances[idx] = static_cast<float>(fvec.z);

		Transform::matrix4_type lightViewProj(lightCamera->projection() * lightCamera->globalTransformInverse());
		lightFrustumMatrices[idx] = math::Matrix4f((biasMatrix * lightViewProj) * cameraViewMatrix);
	}
	glPolygonOffset(oldPolygonOffsetFactor, oldPolygonOffsetUnits);
	if(!polygonOffsetFillEnabled)
		glDisable(GL_POLYGON_OFFSET_FILL);

	//scene::Material::unbind();

	FramebufferObject::Disable();


	rs->get_p_buffer()->Bind();
	rs->get_p_buffer()->BindDrawBuffers();

	// render the lighting quad
	scene::Material::ptr lightMat = lightParams->get_light_material();

	Viewport::ptr viewport = mainCamera->get_viewport();
	scene::Geometry::ptr quadGeometry = utils::create_new_screen_quad(0, 0, viewport->width(), viewport->height());
	quadGeometry->set_material(lightMat);

	lightMat->set_parameter("ShadowDepthTextures", rs->get_cascade_shadow_depth_texture());
	lightMat->set_parameter("FarDistances", farDistances);
	lightMat->set_parameter("LightFrustumCount", (int)lightFrustumMatrices.size());
	lightMat->set_parameter("LightFrustum", lightFrustumMatrices);
	//lightMat->set_parameter("LightFrustum1", lightFrustumMatrices[1]);
	//lightMat->set_parameter("LightFrustum2", lightFrustumMatrices[2]);
	//lightMat->set_parameter("LightFrustum3", lightFrustumMatrices[3]);

	math::Vector3f cameraLocalLightDir//(light->get_dir());
		(mainCamera->localiseV(Transform::vec3_type(lightDir)));
	lightMat->set_parameter("LightDir", cameraLocalLightDir);

	//math::Matrix4d modelView = mainCamera->globalTransformInverse() *
	//	lightParams->get_atmosphere_center_transform()->globalTransform();
	//math::Vector3f eyeSpacePlanetCenter(modelView * math::Vector4d::WAxis);
	//lightMat->set_parameter("PlanetCenter", eyeSpacePlanetCenter);

	glViewport(0, 0, viewport->width(), viewport->height());

	transform::Camera::ptr camera(new transform::Camera());
	camera->set_type(transform::ProjectionType::Orthographic);
	camera->set_near_plane(Transform::float_type(-1.0));	
	camera->set_far_plane(Transform::float_type(1.0));
	camera->set_viewport(viewport);
	camera->set_render_area(Camera::rect_type(0, 0, viewport->width(), viewport->height()));
	
	//GeometryWithRange geom(quadGeometry);
	draw_geometry(quadGeometry, camera);

	FramebufferObject::Disable();
	//scene::Material::unbind();
}


void render_geometry_render_stage( GeometryRenderStage::ptr rs, const SceneContext& sc )
{
	// enable fbo target if there is one
	if(rs->get_fbo_target() == NULL)
	{
		//FramebufferObject::Disable();
		//FramebufferObject::BindDefaultDrawBuffer();
	}
	else
	{
		CHECK_OPENGL_ERRORS;
		rs->get_fbo_target()->Bind();
		if(rs->get_render_targets().empty())
			rs->get_fbo_target()->BindDrawBuffers();
		else
			glDrawBuffers(static_cast<GLsizei>(rs->get_render_targets().size()), &rs->get_render_targets()[0]);
		CHECK_OPENGL_ERRORS;
	}

	const SortedGeometrySet& geometrySet = get_geometry_set(rs->get_geometry(), rs->get_camera(), rs->is_flag_set(RenderStageFlags::FrustumCull));

	scene::Viewport::ptr viewport = rs->get_camera()->get_viewport();
	// setup the get_viewport
	glViewport(viewport->left, viewport->bottom, viewport->right - viewport->left, viewport->top - viewport->bottom);
	CHECK_OPENGL_ERRORS;

	// clear the screen if required
	if(rs->is_flag_set(RenderStageFlags::ClearColour))
	{
		glClearColor(rs->get_clear_colour().x, rs->get_clear_colour().y, 
			rs->get_clear_colour().z, rs->get_clear_colour().w);
		if(rs->is_flag_set(RenderStageFlags::ClearDepth))
			glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		else
			glClear(GL_COLOR_BUFFER_BIT);
	}
	else if(rs->is_flag_set(RenderStageFlags::ClearDepth))
		glClear(GL_DEPTH_BUFFER_BIT);

	CHECK_OPENGL_ERRORS;

	if(!geometrySet.geom.empty())
	{
		if(rs->is_flag_set(RenderStageFlags::WireFrame))
			glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		scene::transform::Camera::ptr camera = rs->get_camera();
		math::Planef globalCameraPlane(camera->forwardGlobal(), camera->centerGlobal());

		if(rs->is_flag_set(RenderStageFlags::OptimizeZBuffer))
		{
 			camera->set_near_plane(geometrySet.range.neard);
 			camera->set_far_plane(geometrySet.range.fard);
		}
		draw_depth_range(geometrySet.geom.begin(), geometrySet.geom.end(), camera);

		if(rs->is_flag_set(RenderStageFlags::WireFrame))
			glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		rs->call_render_stage_complete();

		//_vmm->finalizeRender();
	}
	CHECK_OPENGL_ERRORS;
	if(rs->get_fbo_target() != NULL)
	{
		FramebufferObject::Disable();
		CHECK_OPENGL_ERRORS;
		//FramebufferObject::BindDefaultDrawBuffer();
	}
}

void render_lighting_render_stage( LightingRenderStage::ptr rs, const SceneContext& sc )
{
	// rendering the lights needs the source g buffer and target p buffer

	// Each light render consists of:
	// a) determine set of objects visible to the light, if it is a directional light (i.e. a sun) then this is everything.
	// b) if it is a directional light create cascaded shadow maps, 
	// if it is a point light create shadow cube map,
	// if it is a spot light create a standard 2D shadow map.
	// All shadow maps of the same type should share textures to save memory.
	// c) determine screen space rectangle that encloses entire set of lit objects, or full screen whichever is smaller
	// d) render screen space rectangle using appropriate shader for the light type.

	// clear the screen if required
	if(rs->is_flag_set(RenderStageFlags::ClearColour))
	{
		rs->get_p_buffer()->Bind();
		rs->get_p_buffer()->BindDrawBuffers();
		scene::Viewport::ptr viewport = rs->get_geometry_stage()->get_camera()->get_viewport();
		glViewport(viewport->left, viewport->bottom, viewport->width(), viewport->height());
		glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
		glClear(GL_COLOR_BUFFER_BIT);
		FramebufferObject::Disable();
	}

	for(LightingRenderStage::ConstLightIterator lItr = rs->begin_lights();
		lItr != rs->end_lights(); ++lItr)
	{
		LightingRenderStage::LightParams::ptr lightParams = *lItr;
		switch(lightParams->get_light()->get_type())
		{
		case LightType::Solar:
			render_solar_light(rs, lightParams);
			break;
		case LightType::Directional:
		case LightType::Point:
		case LightType::Spot:
		default:
			break;
		}
	}
}

void render_atmospheric_render_stage(AtmosphericsRenderStage::ptr rs, const SceneContext& sc)
{
	// Atmospheric attenuation applies color modulation based on distance through the atmosphere.
	// Only one attenuation is required for a each atmosphere rendered regardless of number of light sources.
	// The result is a multiplication of the base lit surface color. 
	// Input is g buffer and p buffer.
	// First copy p buffer to output
	// Then render attenuation shapes to output with blending set to multiply
	// Therefore it requires two p buffers, one with the source and one with the target. 

	scene::Viewport::ptr viewport = rs->get_geometry_stage()->get_camera()->get_viewport();

	//::glBindBuffer(GL_READ_BUFFER, rs->get_p_buffer()->I
	rs->get_p_buffer()->BindRead();
	rs->get_fbo_target()->BindDraw();
	::glBlitFramebuffer(0, 0, viewport->width(), viewport->height(), 0, 0, viewport->width(), viewport->height(), 
			GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT, GL_NEAREST);

	for(AtmosphericsRenderStage::ConstAtmosphereIterator aItr = rs->begin_attenuates(); aItr != rs->end_attenuates(); ++aItr)
	{
		AtmosphericsRenderStage::AtmosphereParams::ptr attenuate = *aItr;

	}
	FramebufferObject::Disable();
}

void render_ui_render_stage(const UIRenderStage::ptr& rs)
{
	auto contexts = rs->get_contexts();
	for(auto itr = contexts.begin(); itr != contexts.end(); ++itr)
	{
		vice::Renderer::render(*itr->component);
	}
}

/* 
To draw meshes with optimized Z:
- split meshes into near and far set.
- determine near and far z for near and far set.
- split near and far sets into translucent and opaque
- sort opaque sets from near to far, sort trans sets from far to near
- draw far opaque, draw far trans, draw near opaque, draw near trans.
*/

template < class GItr_ >
DepthRange determine_depth_extents(GItr_ begin, GItr_ end, float minD, float maxD)
{
	DepthRange depth;
	for(GItr_ itr = begin; itr != end; ++itr)
	{
		depth.neard = math::clamp(itr->range.neard, minD, depth.neard);
		depth.fard = math::clamp(itr->range.fard, depth.fard, maxD);
	}

	return depth;
}

template < class GItr_ >
void split_near_and_far(GItr_ beginGeom, GItr_ endGeom, UnsortedGeometrySet& nearSet,
						UnsortedGeometrySet& farSet)
{
	for(GItr_ gItr = beginGeom; gItr != endGeom; ++gItr)
	{
		GeometryWithRange& geom = *gItr;
		if(geom.geometry->is_near())
			nearSet.insert(geom);
		else
			farSet.insert(geom);
	}
}

template < class GItr_ >
void split_trans_not_trans(GItr_ beginGeom, GItr_ endGeom, UnsortedGeometrySet& transSet,
						   UnsortedGeometrySet& notTransSet)
{
	notTransSet.insert(beginGeom, endGeom);  // need to add transparency back to materials..
}

template < class GItr_ >
void sort_trans_set(GItr_ beginGeom, GItr_ endGeom, SortedGeometryFTBSet& transSet)
{
	transSet.insert(beginGeom, endGeom);
}

template < class GItr_ >
void sort_non_trans_set(GItr_ beginGeom, GItr_ endGeom, SortedGeometryBTFSet& transSet)
{
	transSet.insert(beginGeom, endGeom);
}

void apply_symantic_param(const std::string& symName, 
						   const SemanticParamSetMap& symanticMap, 
						   effect::Effect::ptr effect,
						   scene::transform::Transform::ptr meshTrans,
						   scene::transform::Camera::ptr camera,
						   effect::Effect::EffectMode::type effectMode)
{
	SemanticParamSetMap::ParamSetFunction setFn = 
		symanticMap.get_semantic_bind_fn(symName);
	if(setFn)
		setFn(effect, symName, meshTrans, camera, effectMode);
}

void apply_symantic_params(effect::Effect::ptr effect, 
								   scene::transform::Transform::ptr meshTrans, 
								   scene::transform::Camera::ptr camera, 
								   effect::Effect::EffectMode::type effectMode )
{
	effect->for_each_symantic(std::bind(apply_symantic_param, std::placeholders::_1, 
		std::ref(_semanticMatrixSetMap), effect, meshTrans, camera, effectMode));
}

void draw_geometry(const scene::Geometry::ptr& geometry, const scene::transform::Camera::ptr& camera, effect::Effect::EffectMode::type effectMode)
{
	CHECK_OPENGL_ERRORS;
	effect::Effect::ptr eff = geometry->get_material()->get_effect();
	CHECK_OPENGL_ERRORS;
	geometry->get_material()->bind(effectMode);
	CHECK_OPENGL_ERRORS;
	apply_symantic_params(eff, geometry->get_transform(), camera, effectMode);

	++ _shaderBinds;

	CHECK_OPENGL_ERRORS;
	
	if(glbase::VideoMemoryManager::load_buffers(geometry->get_tris(), geometry->get_verts()))
	{
		CHECK_OPENGL_ERRORS;
		glbase::VideoMemoryManager::render_current();
		CHECK_OPENGL_ERRORS;
		glbase::VideoMemoryManager::unbind_buffers();
	}

	scene::Material::unbind();
	CHECK_OPENGL_ERRORS;
}

void create_basic_geoms()
{
	_sphere = scene::Geometry::ptr(new scene::Geometry());
}

};

void RenderGL::render( const SceneContext& sc )
{
	typedef std::vector< RenderStage::ptr > RenderStageVector;
	RenderStageVector stages;

	_geometrySets.clear();

	insert_stages(stages, sc.beginStages(), sc.endStages());

	if(stages.size() != 0)
	{

		for(size_t idx = 0; idx < stages.size(); ++idx)
		{
			const RenderStage::ptr& rs = stages[idx];
			switch(rs->get_type())
			{
			case RenderStage::StageType::Geometry:
				render_geometry_render_stage(std::dynamic_pointer_cast<GeometryRenderStage>(rs), sc);
				break;
			case RenderStage::StageType::Lighting:
				render_lighting_render_stage(std::dynamic_pointer_cast<LightingRenderStage>(rs), sc);
				break;
// 			case RenderStage::StageType::Atmosphere:
// 				render_atmospheric_render_stage(std::dynamic_pointer_cast<AtmosphericsRenderStage>(rs), sc);
// 				break;
			case RenderStage::StageType::UI:
				render_ui_render_stage(std::dynamic_pointer_cast<UIRenderStage>(rs));
				break;
			};
		}
		//scene::Material::unbind();
		
		//std::for_each(stages.begin(), stages.end(), 
		//	std::bind(&render_geometry_render_stage, this, std::placeholders::_1, boost::ref(sc)));
	}
}

void RenderGL::draw_text(int x, int y, const std::string& str, const FontGL& font)
{
	//_fontGeometry->get_transform()->setTransform(math::translate<double>(x, y, 0));
	using namespace scene::transform;

	Camera::ptr camera(new Camera());
	camera->set_type(ProjectionType::Orthographic);
	camera->set_near_plane(scene::transform::Transform::float_type(-1.0));	
	camera->set_far_plane(scene::transform::Transform::float_type(1.0));
	scene::Viewport::ptr viewport(new scene::Viewport(0, 0, glbase::SDLGl::width(), glbase::SDLGl::height()));
	camera->set_viewport(viewport);
	camera->set_render_area(Camera::rect_type(0, 0, glbase::SDLGl::width(), glbase::SDLGl::height()));

	_fontGeometry->set_material(font.get_material());

	if(str.length() * 6 >= _fontGeometry->get_tris()->get_count())
		return ;

	_fontGeometry->get_tris()->set_active_range(0, (int)str.length() * 6);

// 	{
// 		FontVertex* vertt = _fontGeometry->get_verts()->extract<FontVertex>(0);
// 		FontVertex* vertb = _fontGeometry->get_verts()->extract<FontVertex>(1);
// 	}
// 	

	// NOT OPTIMAL! Dirty just a range... Add map buffer to verts/tri set...
	int xa = x, ya = y;
	for(size_t idx = 0; idx < str.length(); ++idx, xa += 16)
	{
		math::Rectanglef uvs = font.get_char_uvs(str[idx]);
		FontVertex* vertbl = _fontGeometry->get_verts()->extract<FontVertex>(idx * 4 + 0);
		vertbl->pos.x = (float)xa;
		vertbl->pos.y = (float)ya;
		vertbl->uv.x = uvs.left;
		vertbl->uv.y = uvs.bottom;
		FontVertex* vertbr = _fontGeometry->get_verts()->extract<FontVertex>(idx * 4 + 1);
		vertbr->pos.x = (float)(xa + 16);
		vertbr->pos.y = (float)ya;
		vertbr->uv.x = uvs.right;
		vertbr->uv.y = uvs.bottom;
		FontVertex* verttr = _fontGeometry->get_verts()->extract<FontVertex>(idx * 4 + 2);
		verttr->pos.x = (float)(xa + 16);
		verttr->pos.y = (float)(ya + 16);
		verttr->uv.x = uvs.right;
		verttr->uv.y = uvs.top;
		FontVertex* verttl = _fontGeometry->get_verts()->extract<FontVertex>(idx * 4 + 3);
		verttl->pos.x = (float)xa;
		verttl->pos.y = (float)(ya + 16);
		verttl->uv.x = uvs.left;
		verttl->uv.y = uvs.top;
	}
	_fontGeometry->get_verts()->sync_range(0, str.length() * 4);

	draw_geometry(_fontGeometry, camera);
}

std::shared_ptr<void> RenderGL::static_init()
{
	using namespace scene;


	VertexSpec::ptr vertSpec(new VertexSpec());
	vertSpec->append(VertexData::PositionData, 0, sizeof(float), 2, VertexElementType::Float);
	vertSpec->append(VertexData::TexCoord0, 1, sizeof(float), 2, VertexElementType::Float);
	assert(sizeof(FontVertex) == vertSpec->vertexSize());

	VertexSet::ptr verts(new VertexSet(vertSpec, FONT_MAX_CHARS * 4));
	TriangleSet::ptr tris(new TriangleSet(TrianglePrimitiveType::TRIANGLES));

	for(TriangleSet::value_type idx = 0; idx < FONT_MAX_CHARS; ++idx)
	{
		tris->push_back(idx * 4 + 0);
		tris->push_back(idx * 4 + 1);
		tris->push_back(idx * 4 + 2);
		tris->push_back(idx * 4 + 0);
		tris->push_back(idx * 4 + 2);
		tris->push_back(idx * 4 + 3);
	}
	tris->sync_all();

	transform::Transform::ptr trans(new transform::Transform());
	_fontGeometry.reset(new Geometry(tris, verts, Material::ptr(), trans));

	return std::shared_ptr<void>(nullptr, [](void*) { RenderGL::static_release(); });
}

void RenderGL::static_release() 
{
	_geometrySets.clear();
	_sphere.reset();
	_fontGeometry.reset();
}

}