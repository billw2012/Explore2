
#include "rendergl.h"

#include <limits>


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

namespace render{;

/*

To render:

- cull by full frustum
- split between near and far
- split near and far between trans and not trans
- sort trans from far to near, sort not trans from near to far
- render each set

*/

RenderGL::RenderGL() 
	: _vmm(), 
	_nearMaxExtents(1.0f, std::numeric_limits<float>::max()),
	_farMaxExtents(100.0f, std::numeric_limits<float>::max())
{

}

void RenderGL::setVMM( VideoMemoryManager::ptr vmm )
{
	_vmm = vmm;
}

void RenderGL::render( const SceneContext& sc )
{
	typedef std::vector< RenderStage::ptr > RenderStageVector;
	RenderStageVector stages;

	insert_stages(stages, sc.beginStages(), sc.endStages());

	if(stages.size() != 0)
	{
		std::for_each(stages.begin(), stages.end(), 
			boost::bind(&RenderGL::render_render_stage, this, _1, boost::ref(sc)));

		scene::Material::unbind();
	}
}

void add_geometry_to_set_with_exclusion(const scene::Geometry::ptr& geometry, 
	UnsortedGeometrySet& geometrySet, RenderStage::ptr rs)
{
	if(geometry->is_valid() && !rs->excludes_geometry(geometry))
		geometrySet.insert(geometry);
}

void add_geometry_to_set_without_exclusion(const scene::Geometry::ptr& geometry, 
	UnsortedGeometrySet& geometrySet)
{
	if(geometry->is_valid())
		geometrySet.insert(geometry);
}


void RenderGL::render_render_stage( RenderStage::ptr rs, const SceneContext& sc )
{
	// enable fbo target if there is one
	if(rs->get_fbo_target() == NULL)
		FramebufferObject::Disable();
	else
		rs->get_fbo_target()->Bind();

	// first depth sort triangle meshes 
	UnsortedGeometrySet geometries;

	if(!rs->included_geometry_only())
		std::for_each(sc.begin_geometries(), sc.end_geometries(), 
			boost::bind(add_geometry_to_set_with_exclusion, _1, boost::ref(geometries), rs));

	// add meshes that are included by this renderstage
	std::for_each(rs->begin_included_geometries(), rs->end_included_geometries(), 
			boost::bind(add_geometry_to_set_without_exclusion, _1, boost::ref(geometries)));

	unsigned int bindcount = 0;

	if(rs->get_wire_frame())
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

	glDepthFunc(GL_LEQUAL);
	// draw each camera to its buffer
	for(RenderStage::CameraIterator citr = rs->begin_cameras(); citr != rs->end_cameras(); ++citr)
	{
		scene::transform::Camera::ptr camera = citr->second;
		GLenum drawBuffer = citr->first;

		// draw final set from camera
		if(_boundFBO != NULL)
			glDrawBuffer(drawBuffer);
		// setup the get_viewport
		glViewport(rs->get_viewport().left, rs->get_viewport().bottom, 
			rs->get_viewport().right - rs->get_viewport().left, 
			rs->get_viewport().top - rs->get_viewport().bottom);

		// clear the screen if required
		if(rs->get_clear_screen())
		{
			glClearColor(rs->get_clear_colour().x, rs->get_clear_colour().y, 
				rs->get_clear_colour().z, rs->get_clear_colour().w);
			if(rs->get_clear_depth())
				glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
			else
				glClear(GL_COLOR_BUFFER_BIT);
		}
		else if(rs->get_clear_depth())
			glClear(GL_DEPTH_BUFFER_BIT);

		if(!geometries.empty())
		{
			math::Planef globalCameraPlane(camera->forwardGlobal(), camera->centerGlobal());

			//if(rs->get_optimize_z_buffer())
			//	draw_geometries_optimize_z(geometries, globalCameraPlane, /*materialSet,*/ camera, rs);
			//else
				draw_geometries(geometries, globalCameraPlane, /*materialSet,*/ camera, rs);
		}

		_vmm->finalizeRender();

	}

	if(rs->get_wire_frame())
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

	rs->call_render_stage_complete();
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
void add_cull_and_calculate_distances(GItr_ beginGeom, GItr_ endGeom, 
									  scene::transform::Camera::ptr camera,
									  float minNearPlane, float maxFarPlane,
									  UnsortedGeometrySet& culledGeoms,
									  bool frustumCull)
{
	math::Frustumf frustum(
		math::Vector3f(camera->forwardGlobal()), 
		math::Vector3f(camera->rightGlobal()), 
		math::Vector3f(camera->upGlobal()), 
		math::Vector3f(camera->centerGlobal()), camera->getFOV(), 
		camera->getFOVH(), minNearPlane, maxFarPlane);

	math::Planef globalCameraPlane(math::Vector3f(camera->forwardGlobal()), 
		math::Vector3f(camera->centerGlobal()));

	for(GItr_ gItr = beginGeom; gItr != endGeom; ++gItr)
	{
		const GeometryWithRange& geom = *gItr;

		math::BoundingSpheref globalBS(
			geom.geometry->get_transform()->globalise(
			math::BoundingSphered(geom.geometry->get_bsphere())));

		if(!frustumCull || math::intersects(globalBS, frustum).occured)
		{
			GeometryWithRange culledGeom(geom);
			culledGeom.distance = globalCameraPlane.distance(globalBS.center());
			culledGeom.range.neard = culledGeom.distance - globalBS.radius();
			culledGeom.range.fard = culledGeom.distance + globalBS.radius();
			culledGeoms.insert(culledGeom);
		}		
	}
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

//void RenderGL::draw_geometries_optimize_z(const UnsortedGeometrySet &geometrySet, 
//									   const math::Planef& globalCameraPlane, 
//									   scene::transform::Camera::ptr camera, 
//									   RenderStage::ptr rs)
//{
//	UnsortedGeometrySet culledGeometrySet;
//	add_cull_and_calculate_distances(geometrySet.begin(), geometrySet.end(), 
//		camera, _nearMaxExtents.neard, _farMaxExtents.fard, culledGeometrySet);
//	//UnsortedGeometrySet /*nearGeometrySet,*/ farGeometrySet();
//	//split_near_and_far(culledGeometrySet.begin(), culledGeometrySet.end(), nearGeometrySet,
//	//	farGeometrySet);
//	//DepthRange nearExtents = determine_depth_extents(nearGeometrySet.begin(),
//	//	nearGeometrySet.end(), _nearMaxExtents.neard, _nearMaxExtents.fard/*_minNearPlane, _maxFarPlane*/);
//	DepthRange farExtents = determine_depth_extents(/*farGeometrySet*/culledGeometrySet.begin(),
//		/*farGeometrySet*/culledGeometrySet.end(), _nearMaxExtents.neard, _farMaxExtents.fard/*_minNearPlane, _maxFarPlane*/);
//
//	//UnsortedGeometrySet nearTransSet, nearNotTransSet;
//	//split_trans_not_trans(nearGeometrySet.begin(), nearGeometrySet.end(), nearTransSet, 
//	//	nearNotTransSet);
//
//	UnsortedGeometrySet farTransSet, farNotTransSet;
//	split_trans_not_trans(/*farGeometrySet*/culledGeometrySet.begin(), /*farGeometrySet*/culledGeometrySet.end(), farTransSet, 
//		farNotTransSet);
//
//	SortedGeometryFTBSet /*sortedNearTransSet,*/ sortedFarTransSet;
//	//sort_trans_set(nearTransSet.begin(), nearTransSet.end(), sortedNearTransSet);
//	sort_trans_set(farTransSet.begin(), farTransSet.end(), sortedFarTransSet);
//
//	SortedGeometryBTFSet /*sortedNearNotTransSet,*/ sortedFarNotTransSet;
//	//sort_non_trans_set(nearNotTransSet.begin(), nearNotTransSet.end(), sortedNearNotTransSet);
//	sort_non_trans_set(farNotTransSet.begin(), farNotTransSet.end(), sortedFarNotTransSet);
//
//	if(!farNotTransSet.empty() || !farTransSet.empty()) 
//	{
//		camera->setNearPlane(/*1.0f);*/farExtents.neard);
//		camera->setFarPlane(/*100000000.0f);*/farExtents.fard);
//		draw_depth_range(farNotTransSet.begin(), farNotTransSet.end(),
//			farTransSet.begin(), farTransSet.end(), camera);
//
//		//if(!nearNotTransSet.empty() || !nearTransSet.empty()) 
//		//	glClear(GL_DEPTH_BUFFER_BIT);
//	}
//
//	//if(!nearNotTransSet.empty() || !nearTransSet.empty()) 
//	//{
//	//	camera->setNearPlane(1.0f);//nearExtents.neard);
//	//	camera->setFarPlane(100000000.0f);//nearExtents.fard);
//	//	draw_depth_range(nearNotTransSet.begin(), nearNotTransSet.end(),
//	//		nearTransSet.begin(), nearTransSet.end(), camera);
//	//}
//}

void RenderGL::draw_geometries(const UnsortedGeometrySet &geometrySet, 
										  const math::Planef& globalCameraPlane, 
										  scene::transform::Camera::ptr camera, 
										  RenderStage::ptr rs)
{
	const UnsortedGeometrySet* pCulledGeometrySet = &geometrySet;
	UnsortedGeometrySet culledGeometrySet;
	if(rs->get_frustum_cull() || rs->get_optimize_z_buffer())
	{
		add_cull_and_calculate_distances(geometrySet.begin(), geometrySet.end(), 
			camera, _nearMaxExtents.neard, _farMaxExtents.fard, culledGeometrySet,
			rs->get_frustum_cull());
		pCulledGeometrySet = &culledGeometrySet;
	}

	DepthRange farExtents = determine_depth_extents(pCulledGeometrySet->begin(),
		pCulledGeometrySet->end(), _nearMaxExtents.neard, _farMaxExtents.fard);

	UnsortedGeometrySet farTransSet, farNotTransSet;
	split_trans_not_trans(pCulledGeometrySet->begin(), pCulledGeometrySet->end(), farTransSet, 
		farNotTransSet);

	SortedGeometryFTBSet sortedFarTransSet;
	sort_trans_set(farTransSet.begin(), farTransSet.end(), sortedFarTransSet);

	SortedGeometryBTFSet sortedFarNotTransSet;
	sort_non_trans_set(farNotTransSet.begin(), farNotTransSet.end(), sortedFarNotTransSet);

	if(!farNotTransSet.empty() || !farTransSet.empty()) 
	{
		if(rs->get_optimize_z_buffer())
		{
			camera->setNearPlane(farExtents.neard);
			camera->setFarPlane(farExtents.fard);
		}
		draw_depth_range(farNotTransSet.begin(), farNotTransSet.end(),
			farTransSet.begin(), farTransSet.end(), camera);
	}
}


void apply_symantic_matrix(const std::string& symName, 
						   const SemanticMatrixSetMap& symanticMap, 
						   effect::Effect::ptr effect,
						   scene::transform::Transform::ptr meshTrans,
						   scene::transform::Camera::ptr camera)
{
	SemanticMatrixSetMap::MatrixSetFunction setFn = 
		symanticMap.get_semantic_bind_fn(symName);
	if(setFn)
		setFn(effect, symName, meshTrans, camera);
}

void RenderGL::apply_matrix_params(effect::Effect::ptr effect, 
								   scene::transform::Transform::ptr meshTrans, 
								   scene::transform::Camera::ptr camera )
{
	effect->for_each_symantic(boost::bind(apply_symantic_matrix, _1, 
		boost::ref(_semanticMatrixSetMap), effect, meshTrans, camera));
}

void RenderGL::draw_geometry(const GeometryWithRange& geom, scene::transform::Camera::ptr camera)
{
	//geom.geometry->get_material()->get_effect()->update_parameters();
	effect::Effect::ptr eff = geom.geometry->get_material()->get_effect();
	//eff->clear_active_texture_units();
	geom.geometry->get_material()->bind();
	apply_matrix_params(eff, geom.geometry->get_transform(), camera);

	//geom.geometry->get_material()->bind();

	++ _shaderBinds;
	
	if(_vmm->loadTriangleSet<scene::TriangleSet::value_type>(geom.geometry->get_tris(),
		geom.geometry->get_verts()))
		_vmm->renderTriangleSet();

	//eff->deactivate_active_texture_units();
}

}