#ifndef _RENDER_RENDERGL_HPP
#define _RENDER_RENDERGL_HPP

#include <stdexcept>
#include <set>
#include <map>
#include <functional>
#include <boost/iterator/transform_iterator.hpp>
#include "Math/frustum.hpp"
#include "Math/intersection.hpp"
#include "GLBase/sdlgl.hpp"
#include "videomemorymanager.hpp"
#include "scenecontext.hpp"
#include "semantic_matrix_set.hpp"

#include "RenderDLL.h"

namespace render {;

template < class PTy > 
bool operator < (const boost::shared_ptr< PTy >& left, const boost::shared_ptr< PTy >& right) 
{
	return left.get() < right.get();
}

template < class PTy > 
bool operator > (const boost::shared_ptr< PTy >& left, const boost::shared_ptr< PTy >& right) 
{
	return left.get() > right.get();
}

struct DepthRange
{
	DepthRange(float near_ = std::numeric_limits<float>::max(), 
		float far_ = -std::numeric_limits<float>::max()) : neard(near_), fard(far_) {}
	float neard, fard;
};

struct GeometryWithRange
{
	GeometryWithRange(scene::Geometry::ptr geom = scene::Geometry::ptr()) 
		: geometry(geom), distance(0), range(0, 0) {}

	scene::Geometry::ptr geometry;
	float distance;
	DepthRange range;

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

		return left.distance < right.distance;
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

		return left.distance > right.distance;
	}
};

typedef std::set< GeometryWithRange, GeometryWithDistanceLessThanFTB > 
	SortedGeometryFTBSet;
typedef std::set< GeometryWithRange, GeometryWithDistanceLessThanBTF > 
	SortedGeometryBTFSet;
typedef std::set< GeometryWithRange > UnsortedGeometrySet;

struct RENDER_API RenderGL
{
private:

	//struct SortedTriangleMesh
	//{
	//	struct TriangleSetSort
	//	{
	//		math::Planef localCameraPlane;

	//		TriangleSetSort(const math::Planef& localCam) : localCameraPlane(localCam) {}

	//		bool operator()(scene::TriangleSet::ptr left, scene::TriangleSet::ptr right) const
	//		{
	//			float leftDist = localCameraPlane.distance(left->aabb().center());
	//			float rightDist = localCameraPlane.distance(right->aabb().center());
	//			return leftDist < rightDist;
	//		}
	//	};

	//	typedef boost::shared_ptr< SortedTriangleMesh > ptr;
	//	typedef std::vector< scene::TriangleSet::ptr > TriangleSetSet;

	//	scene::transform::MeshTransform::ptr meshTrans;
	//	TriangleSetSet tris;

	//	SortedTriangleMesh(ptr mesh_, const math::Planef& localCamera, bool sorttris)
	//		: meshTrans(mesh_->meshTrans)
	//	{
	//		tris.insert(tris.begin(), mesh_->tris.begin(), mesh_->tris.end());
	//		if(sorttris)
	//			std::sort(tris.begin(), tris.end(), TriangleSetSort(localCamera));
	//	}

	//	SortedTriangleMesh(scene::transform::MeshTransform::ptr mesh_)
	//		: meshTrans(mesh_)
	//	{
	//	}

	//	SortedTriangleMesh(scene::transform::MeshTransform::ptr meshTrans_, 
	//		const math::Planef& localCamera, 
	//		const math::Frustumf& frustum, 
	//		bool fcull, bool sorttris)
	//		: meshTrans(meshTrans_)
	//	{
	//		assert(meshTrans->get_mesh() != NULL);
	//		scene::Mesh::ptr mesh = meshTrans->get_mesh();
	//		if(fcull)
	//		{
	//			for(scene::Mesh::TriangleSetIterator tsItr = mesh->beginTris(); tsItr != mesh->endTris(); ++ tsItr)
	//				if(math::intersects(meshTrans->globalise((*tsItr)->bsphere()), frustum).occured)
	//					tris.push_back(*tsItr);
	//		}
	//		else
	//			tris.insert(tris.begin(), mesh->beginTris(), mesh->endTris());
	//		if(sorttris)
	//			std::sort(tris.begin(), tris.end(), TriangleSetSort(localCamera));
	//	}
	//};

	//typedef std::vector< SortedTriangleMesh::ptr > SortedTriangleMeshSet;

private:
	VideoMemoryManager::ptr _vmm;
	FramebufferObject::ptr _boundFBO;

	DepthRange _nearMaxExtents, _farMaxExtents;
	//float _minNearNearPlane, _maxNearFarPlane, _minNearNearPlane, _maxNearFarPlane;
	
	// diagnostic vars
	unsigned int _shaderBinds;

	SemanticMatrixSetMap _semanticMatrixSetMap;

	bool _depthSplitEnabled;

	// WARNING: This is bad, if time gets too high it will become unstable, need to either rotate it or use unsigned int, but that is more difficult to interface with shaders, so use float temporarily
public:
	RenderGL();

	void setVMM(VideoMemoryManager::ptr vmm);

	template < class RenderStageVectorTy_, class RSItr_ >
	void insert_stages(RenderStageVectorTy_& stages, RSItr_ start, RSItr_ end);

	// to render with multiple lights:
	// for each light determine the meshes it effects
	// render a shadow map using those objects as shadow casters
	// for each mesh render using active lights and shadow maps for the lights
	void render(const SceneContext& sc);

private:

	//typedef std::set< scene::transform::MeshTransform::ptr > TriangleMeshSet;
	//typedef std::set< typename scene::Material::ptr > MaterialSet;

	void render_render_stage(RenderStage::ptr rs, const SceneContext& sc);

	//struct GUIClipData
	//{
	//	GUIClipData(const GUIClipData& base, gui::Object::ptr clipObject)
	//	{
	//		using namespace math;
	//		bl = Vector3f(clipObject->get_transform()->globalise(Vector3d::Zero));
	//		br = Vector3f(clipObject->get_transform()->globalise(Vector3d::XAxis * clipObject->get_width()));
	//		tl = Vector3f(clipObject->get_transform()->globalise(Vector3d::YAxis * clipObject->get_height()));

	//		bl.x = std::max(bl.x, base.bl.x);
	//		bl.y = std::max(bl.y, base.bl.y);
	//		br.x = std::min(br.x, base.br.x);
	//		br.y = std::max(br.y, base.br.y);
	//		tl.x = std::max(tl.x, base.tl.x);
	//		tl.y = std::min(tl.y, base.tl.y);


	//		Vector3f rightVec((br - bl).normal());
	//		Vector3f upVec((tl - bl).normal());

	//		left = Planef(rightVec, bl);
	//		right = Planef(-rightVec, br);
	//		bottom = Planef(upVec, bl);
	//		top = Planef(-upVec, tl);
	//	}

	//	GUIClipData(gui::Object::ptr clipObject)
	//	{
	//		using namespace math;
	//		bl = Vector3f(clipObject->get_transform()->globalise(Vector3d::Zero));
	//		br = Vector3f(clipObject->get_transform()->globalise(Vector3d::XAxis * clipObject->get_width()));
	//		tl = Vector3f(clipObject->get_transform()->globalise(Vector3d::YAxis * clipObject->get_height()));
	//		Vector3f rightVec((br - bl).normal());
	//		Vector3f upVec((tl - bl).normal());

	//		left = Planef(rightVec, bl);
	//		right = Planef(-rightVec, br);
	//		bottom = Planef(upVec, bl);
	//		top = Planef(-upVec, tl);
	//	}

	//	math::Vector3f bl, br, tl;
	//	math::Planef left, right, bottom, top;
	//};

	//void draw_gui(RenderStage::ptr rs);

	//void draw_gui_object(gui::Object::ptr obj, scene::transform::Camera::ptr camera,
	//	GUIClipData& clipData);

	void draw_geometries(const UnsortedGeometrySet& meshSet, 
		const math::Planef& globalCameraPlane, 
		/*MaterialSet& materialSet,*/ 
		scene::transform::Camera::ptr camera, 
		RenderStage::ptr rs);

	//static void add_cull_and_sort_tri_sets(const UnsortedGeometrySet &geometries, 
	//	scene::transform::Camera::ptr camera, const math::Frustumf& frustum, 
	//	RenderStage::ptr rs, SortedGeometrySet &sortedAndCulledGeometries);

	void draw_geometries_optimize_z(const UnsortedGeometrySet &meshSet, 
		const math::Planef& globalCameraPlane, /*MaterialSet& materialSet,*/ 
		scene::transform::Camera::ptr camera, RenderStage::ptr rs);

	template < class NTItr_, class TItr_ > 
	void draw_depth_range(NTItr_ beginNotTrans, NTItr_ endNotTrans, 
		TItr_ beginTrans, TItr_ endTrans, scene::transform::Camera::ptr camera);

	//void add_geometry_to_set(scene::transform::MeshTransform::ptr meshTrans, 
	//	TriangleMeshSet& meshSet);

	//void add_geometry_to_set_with_exclusion(scene::transform::MeshTransform::ptr meshTrans, 
	//	TriangleMeshSet& meshSet, RenderStage::ptr rs);

	//template < class V, class H, class VF, class HF >
	//bool frustumCull(const math::BoundingSphere<V,H>& bsph, 
	//	const math::Frustum<VF,HF>& frustum);

	//template < class GeometrySetTy_ >
	//void drawDepthRange( scene::transform::Camera::ptr camera, 
	//	const GeometrySetTy_ &unsortedMeshes//, 
	//	/*SortedGeometrySet &sortedTranslucentMeshes*/ );

	//template < class MeshItr, class MaterialSetTy_ > 
	//void splitAndSortForTransparency( MeshItr beginMeshes, MeshItr endMeshes, 
	//	const math::Frustumf& frustum, scene::transform::Camera::ptr camera, 
	//	const math::Planef& globalCameraPlane, MaterialSetTy_& materialSet, 
	//	SortedTriangleMeshSet& sortedTranslucentMeshes, SortedTriangleMeshSet& unsortedMeshes, 
	//	RenderStage::ptr rs );
	//template < class MeshItr, class MaterialSetTy_ > 
	//void splitForTransparency( MeshItr beginMeshes, MeshItr endMeshes, 
	//	scene::transform::Camera::ptr camera, const math::Planef& globalCameraPlane, 
	//	MaterialSetTy_& materialSet, SortedTriangleMeshSet& sortedTranslucentMeshes, 
	//	SortedTriangleMeshSet& unsortedMeshes, RenderStage::ptr rs );
	void apply_matrix_params(effect::Effect::ptr effect, 
		scene::transform::Transform::ptr trans, scene::transform::Camera::ptr camera);

	void draw_geometry(const GeometryWithRange& geom, 
		scene::transform::Camera::ptr camera);

	//void drawSortedTriangleMesh(SortedTriangleMesh::ptr sortedTriMesh, 
	//	scene::transform::Camera::ptr camera);
};

template < class RenderStageVectorTy_, class RSItr_ >
void RenderGL::insert_stages(RenderStageVectorTy_& stages, RSItr_ start, RSItr_ end)
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

//template < class V, class H, class VF, class HF >
//bool RenderGL::frustumCull( const math::BoundingSphere<V,H>& bsph, const math::Frustum<VF,HF>& frustum )
//{
//	return !math::intersects(bsph, frustum).occured;
//}

//template < class MeshItr, class MaterialSetTy_ > 
//void RenderGL::splitAndSortForTransparency( MeshItr beginMeshes, MeshItr endMeshes, 
//								 const math::Frustumf& frustum, scene::transform::Camera::ptr camera, 
//								 const math::Planef& globalCameraPlane, MaterialSetTy_& materialSet, 
//								 SortedTriangleMeshSet& sortedTranslucentMeshes, SortedTriangleMeshSet& unsortedMeshes, 
//								 RenderStage::ptr rs )
//{
//	struct SortedTriangleMeshSort
//	{
//		math::Planef globalCameraPlane;
//
//		SortedTriangleMeshSort(const math::Planef& globalCam) : globalCameraPlane(globalCam) {}
//
//		bool operator()(SortedTriangleMesh::ptr left, SortedTriangleMesh::ptr right) const
//		{
//			if(left->meshTrans->transparencySortHint() < right->meshTrans->transparencySortHint())
//				return false;
//			if(left->meshTrans->transparencySortHint() > right->meshTrans->transparencySortHint())
//				return true;
//
//			float leftDist = globalCameraPlane.distance(
//				math::Vector3f(left->meshTrans->globalise(math::Vector3d(left->meshTrans->aabb().center()))));
//			float rightDist = globalCameraPlane.distance(
//				math::Vector3f(right->meshTrans->globalise(math::Vector3d(right->meshTrans->aabb().center()))));
//			return leftDist < rightDist;
//		}
//	};
//
//	for(MeshItr sItr = beginMeshes; sItr != endMeshes; ++sItr)
//	{
//		scene::transform::MeshTransform::ptr meshTrans = *sItr;
//		scene::Mesh::ptr mesh = meshTrans->get_mesh();
//		assert(mesh != NULL);
//		if(!(frustumCull(meshTrans->bsphereGlobal(), frustum) && rs->get_frustum_cull()))
//		{
//			if(mesh->material() && materialSet.find(mesh->material()) == materialSet.end())
//			{
//				scene::ShaderSpecs specs;
//				specs.cameraPosGlobal = math::Vector3f(camera->centerGlobal());
//				mesh->material()->selectShader(specs);	
//				materialSet.insert(mesh->material());
//			}
//
//			math::Planef localCameraPlane(meshTrans->localiseV(camera->forwardGlobal()), 
//				meshTrans->localise(camera->centerGlobal()));
//			// triangle sets are depth sorted as they are inserted if the material is translucent
//			SortedTriangleMesh::ptr sortedMesh(new SortedTriangleMesh(meshTrans, 
//				localCameraPlane, frustum, rs->get_frustum_cull(), mesh->material() && 
//				mesh->material()->shader()->transparency()));
//
//			if(sortedMesh->tris.size() > 0)
//			{
//				if(mesh->material() && mesh->material()->shader()->transparency())
//					sortedTranslucentMeshes.push_back(sortedMesh);
//				else
//					unsortedMeshes.push_back(sortedMesh);
//			}
//		}
//	}
//	// sort meshes
//	std::sort(sortedTranslucentMeshes.begin(), sortedTranslucentMeshes.end(), SortedTriangleMeshSort(globalCameraPlane));
//}

//template < class MeshItr, class MaterialSetTy_ > 
//void RenderGL::splitForTransparency( MeshItr beginMeshes, MeshItr endMeshes, 
//						  scene::transform::Camera::ptr camera, const math::Planef& globalCameraPlane, 
//						  MaterialSetTy_& materialSet, SortedTriangleMeshSet& sortedTranslucentMeshes, 
//						  SortedTriangleMeshSet& unsortedMeshes, RenderStage::ptr rs )
//{
//	struct SortedTriangleMeshSort
//	{
//		math::Planef globalCameraPlane;
//
//		SortedTriangleMeshSort(const math::Planef& globalCam) : globalCameraPlane(globalCam) {}
//
//		bool operator()(SortedTriangleMesh::ptr left, SortedTriangleMesh::ptr right) const
//		{
//			if(left->meshTrans->transparencySortHint() < right->meshTrans->transparencySortHint())
//				return false;
//			if(left->meshTrans->transparencySortHint() > right->meshTrans->transparencySortHint())
//				return true;
//
//			float leftDist = globalCameraPlane.distance(
//				math::Vector3f(left->meshTrans->globalise(math::Vector3d(left->meshTrans->aabb().center()))));
//			float rightDist = globalCameraPlane.distance(
//				math::Vector3f(right->meshTrans->globalise(math::Vector3d(right->meshTrans->aabb().center()))));
//			return leftDist < rightDist;
//		}
//	};
//
//	for(MeshItr sItr = beginMeshes; sItr != endMeshes; ++sItr)
//	{
//		scene::transform::MeshTransform::ptr meshTrans = (*sItr)->meshTrans;
//		scene::Mesh::ptr mesh = meshTrans->get_mesh();
//		assert(mesh != NULL);
//		if(mesh->material() && materialSet.find(mesh->material()) == materialSet.end())
//		{
//			scene::ShaderSpecs specs;
//			specs.cameraPosGlobal = math::Vector3f(camera->centerGlobal());
//			mesh->material()->selectShader(specs);	
//			materialSet.insert(mesh->material());
//		}
//
//		math::Planef localCameraPlane(meshTrans->localiseV(camera->forwardGlobal()), 
//			meshTrans->localise(camera->centerGlobal()));
//		// triangle sets are depth sorted as they are inserted if the material is translucent
//		SortedTriangleMesh::ptr sortedMesh(new SortedTriangleMesh(*sItr, 
//			localCameraPlane, 
//			mesh->material() && mesh->material()->shader()->transparency()));
//
//		if(sortedMesh->tris.size() > 0)
//		{
//			if(mesh->material() && mesh->material()->shader()->transparency())
//				sortedTranslucentMeshes.push_back(sortedMesh);
//			else
//				unsortedMeshes.push_back(sortedMesh);
//		}
//	}
//	// sort meshes
//	std::sort(sortedTranslucentMeshes.begin(), sortedTranslucentMeshes.end(), SortedTriangleMeshSort(globalCameraPlane));
//}


//template < class GItr_ >
//void RenderGL::add_and_cull(GItr_ beginGeom, GItr_ endGeom, const math::Frustumf& frust,
//							UnsortedGeometrySet& culledGeoms)
//{
//	for(GItr_ gItr = beginGeom; gItr != endGeom; ++gItr)
//	{
//		GeometryWithRange& geom = *gItr;
//		geom.geometry->
//			// actually set should be geometry only, no distance until we get to sorting
//		if(frustumCull()
//	}
//}

//template < class GItr_ >
//void RenderGL::split_near_and_far(GItr_ beginGeom, GItr_ endGeom, 
//								  UnsortedGeometrySet& nearSet,
//								  UnsortedGeometrySet& endSet)
//{
//
//}
//
//template < class GItr_ >
//void RenderGL::split_trans_not_trans(GItr_ beginGeom, GItr_ endGeom, 
//									 UnsortedGeometrySet& transSet,
//									 UnsortedGeometrySet& notTransSet)
//{
//
//}
//
//template < class GItr_ >
//void RenderGL::sort_trans_set(GItr_ beginGeom, GItr_ endGeom, 
//							  SortedGeometryFTBSet& transSet)
//{
//
//}
//
//template < class GItr_ >
//void RenderGL::sort_non_trans_set(GItr_ beginGeom, GItr_ endGeom, 
//								  SortedGeometryBTFSet& transSet)
//{
//
//}

template < class NTItr_, class TItr_ > 
void RenderGL::draw_depth_range(NTItr_ beginNotTrans, NTItr_ endNotTrans, 
					  TItr_ beginTrans, TItr_ endTrans,
					  scene::transform::Camera::ptr camera)
{
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();

	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	std::for_each(beginNotTrans, endNotTrans, 
		boost::bind(&RenderGL::draw_geometry, this, _1, camera));
	std::for_each(beginTrans, endTrans, 
		boost::bind(&RenderGL::draw_geometry, this, _1, camera));
}


}

#endif // _RENDER_RENDERGL_HPP