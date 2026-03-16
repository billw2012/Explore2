#if !defined(__RENDER_SEMANTIC_PARAM_SETUP_HPP__)
#define __RENDER_SEMANTIC_PARAM_SETUP_HPP__

#include <string>
#include <functional>
#include "Scene/geometry.hpp"
#include "Scene/camera.hpp"
#include "Effect/effect.h"

using namespace std::placeholders;
//
//#define DEFINE_MAT_FN(_name_, _matfn_) void _name_ (effect::Effect::ptr effect, \
//	const std::string& paramName, scene::transform::Transform::ptr meshTrans, \
//	scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode) \
//	{ effect->set_parameter(paramName, _matfn_ (meshTrans, camera), effectMode); }
//
//#define DEFINE_MAT_I_FN(_name_, _matfn_) \
//	void _name_(effect::Effect::ptr effect, const std::string& paramName, \
//	scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode) { \
//	effect->set_parameter(paramName, _matfn_(meshTrans, camera).inverse(), effectMode); } 
//#define DEFINE_MAT_IT_FN(_name_, _matfn_) \
//	void _name_(effect::Effect::ptr effect, const std::string& paramName, \
//	scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode) { \
//	effect->set_parameter(paramName, _matfn_(meshTrans, camera).inverse().transpose(), effectMode); } 
//#define INSERT_MAT_FN(_semantic_, _fn_) \
//	_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( \
//	_semantic_, std::bind(&SemanticParamSetMap :: _fn_, this, std::placeholders::_1, _2, _3, _4, _5)));
//#define DEFINE_CAMERA_FN(_name_, _camfn_) \
//	void _name_(effect::Effect::ptr effect, const std::string& paramName, \
//	scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode) { \
//	effect->set_parameter(paramName, _camfn_(meshTrans, camera), effectMode); } 
//#define INSERT_CAMERA_FN(_semantic_, _fn_) \
//	_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( \
//	_semantic_, std::bind(&SemanticParamSetMap :: _fn_, this, std::placeholders::_1, _2, _3, _4, _5)));

namespace render { ;

namespace ShaderSymantics 
{
	const std::string MODEL_MAT("MODELMAT");
	const std::string MODEL_MAT_I("MODELMATI");
	const std::string MODEL_MAT_IT("MODELMATIT");
	const std::string VIEW_MAT("VIEWMAT");
	const std::string VIEW_MAT_I("VIEWMATI");
	const std::string VIEW_MAT_IT("VIEWMATIT");
	const std::string PROJECTION_MAT("PROJMAT");
	const std::string PROJECTION_MAT_I("PROJMATI");
	const std::string PROJECTION_MAT_IT("PROJMATIT");
	const std::string MODEL_VIEW_MAT("MODELVIEWMAT");
	const std::string MODEL_VIEW_MAT_I("MODELVIEWMATI");
	const std::string MODEL_VIEW_MAT_IT("MODELVIEWMATIT");
	const std::string MODEL_VIEW_PROJECTION_MAT("MODELVIEWPROJMAT");
	const std::string MODEL_VIEW_PROJECTION_MAT_I("MODELVIEWPROJMATI");
	const std::string MODEL_VIEW_PROJECTION_MAT_IT("MODELVIEWPROJMATIT");
	const std::string VIEWPORT("VIEWPORT");
	const std::string CAMERA_GLOBAL("CAMERAGLOBAL");
	const std::string CAMERA_LOCAL("CAMERALOCAL");
	const std::string NEAR_CLIP("NEARCLIP");
	const std::string FAR_CLIP("FARCLIP");
};

struct SemanticParamSetMap
{
	typedef std::function< 
		void (effect::Effect::ptr effect,
		const std::string&, 
		scene::transform::Transform::ptr, 
		scene::transform::Camera::ptr,
		effect::Effect::EffectMode::type
		) > ParamSetFunction;

	typedef std::unordered_map< std::string, ParamSetFunction > SemanticMatrixSetFunctionMap;

	SemanticParamSetMap()
	{
		build_semantic_param_set_function_map();
	}

	//static void static_init()
	//{
	//	_globalInstance.reset(new SemanticParamSetMap());
	//}

	//static void static_release()
	//{
	//	_globalInstance.reset();
	//}

	//static SemanticParamSetMap* get_instance()
	//{
	//	return _globalInstance.get();
	//}

	ParamSetFunction get_semantic_bind_fn(const std::string& name) const
	{
		auto fnItr = _semanticMatrixSetFunctionMap.find(name);
		if(fnItr != _semanticMatrixSetFunctionMap.end())
			return fnItr->second;
		return ParamSetFunction();
	}

private:
	math::Matrix4f calc_model(scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera)
	{
		return math::Matrix4f(meshTrans->globalTransform());
	}

	math::Matrix4f calc_view(scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera)
	{
		return math::Matrix4f(camera->globalTransformInverse());
	}

	math::Matrix4f calc_proj(scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera)
	{
		return math::Matrix4f(camera->projection());
	}

	math::Matrix4f calc_model_view(scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera)
	{
		return math::Matrix4f((camera->globalTransformInverse() * meshTrans->globalTransform()));
	}

	math::Matrix4f calc_model_view_proj(scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera)
	{
		return math::Matrix4f((camera->projection() * (camera->globalTransformInverse() * meshTrans->globalTransform())));
	}

	math::Vector3f calc_camera_global(scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera)
	{
		return math::Vector3f(camera->centerGlobal());
	}

	math::Vector3f calc_camera_local(scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera)
	{
		return math::Vector3f(meshTrans->localise(camera->centerGlobal()));
	}

	//DEFINE_MAT_FN(model, calc_model);
	//DEFINE_MAT_I_FN(model_i, calc_model);
	//DEFINE_MAT_IT_FN(model_it, calc_model);
	//DEFINE_MAT_FN(view, calc_view);
	//DEFINE_MAT_I_FN(view_i, calc_view);
	//DEFINE_MAT_IT_FN(view_it, calc_view);
	//DEFINE_MAT_FN(proj, calc_proj);
	//DEFINE_MAT_I_FN(proj_i, calc_proj);
	//DEFINE_MAT_IT_FN(proj_it, calc_proj);
	//DEFINE_MAT_FN(model_view, calc_model_view);
	//DEFINE_MAT_I_FN(model_view_i, calc_model_view);
	//DEFINE_MAT_IT_FN(model_view_it, calc_model_view);
	//DEFINE_MAT_FN(model_view_proj, calc_model_view_proj);
	//DEFINE_MAT_I_FN(model_view_proj_i, calc_model_view_proj);
	//DEFINE_MAT_IT_FN(model_view_proj_it, calc_model_view_proj);
	//DEFINE_CAMERA_FN(camera_global, calc_camera_global);
	//DEFINE_CAMERA_FN(camera_local, calc_camera_local);

// 	void near_clip(effect::Effect::ptr effect, const std::string& paramName, 
// 		scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, 
// 		effect::Effect::EffectMode::type effectMode) 
// 	{ 
// 		effect->set_parameter(paramName, static_cast<float>(camera->get_near_plane()), effectMode); 
// 	} 
// 
// 	void far_clip(effect::Effect::ptr effect, const std::string& paramName, 
// 		scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, 
// 		effect::Effect::EffectMode::type effectMode) 
// 	{ 
// 		effect->set_parameter(paramName, static_cast<float>(camera->get_far_plane()), effectMode); 
// 	} 

	void build_semantic_param_set_function_map()
	{
// 		std::function<math::Matrix4f (const math::Matrix4f&) > pass_through_fn = [](const math::Matrix4f& matrix) -> math::Matrix4f { return matrix; };
// 		std::function<math::Matrix4f (const math::Matrix4f&) > inverse_fn = [](const math::Matrix4f& matrix) -> math::Matrix4f { return matrix.inverse(); };
// 		std::function<math::Matrix4f (const math::Matrix4f&) > inverse_transpose_fn = [](const math::Matrix4f& matrix) -> math::Matrix4f { return matrix.inverse().transpose(); };
// 		std::function<math::Matrix4f (const math::Matrix4f&) >[] matrix_fns = { pass_through_fn, inverse_fn, inverse_transpose_fn };

		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::MODEL_MAT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_model (meshTrans, camera), effectMode); }));
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::MODEL_MAT_I, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_model (meshTrans, camera).inverse(), effectMode); }));
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::MODEL_MAT_IT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_model (meshTrans, camera).inverse().transpose(), effectMode); }));
		
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::VIEW_MAT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_view (meshTrans, camera), effectMode); }));
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::VIEW_MAT_I, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_view (meshTrans, camera).inverse(), effectMode); }));
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::VIEW_MAT_IT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_view (meshTrans, camera).inverse().transpose(), effectMode); }));

		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::PROJECTION_MAT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_proj (meshTrans, camera), effectMode); }));
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::PROJECTION_MAT_I, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_proj (meshTrans, camera).inverse(), effectMode); }));
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::PROJECTION_MAT_IT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_proj (meshTrans, camera).inverse().transpose(), effectMode); }));

		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::MODEL_VIEW_MAT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_model_view (meshTrans, camera), effectMode); }));
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::MODEL_VIEW_MAT_I, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_model_view (meshTrans, camera).inverse(), effectMode); }));
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::MODEL_VIEW_MAT_IT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_model_view (meshTrans, camera).inverse().transpose(), effectMode); }));

		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::MODEL_VIEW_PROJECTION_MAT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_model_view_proj (meshTrans, camera), effectMode); }));
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::MODEL_VIEW_PROJECTION_MAT_I, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_model_view_proj (meshTrans, camera).inverse(), effectMode); }));
		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::MODEL_VIEW_PROJECTION_MAT_IT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_model_view_proj (meshTrans, camera).inverse().transpose(), effectMode); }));

		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::CAMERA_GLOBAL, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_camera_global (meshTrans, camera), effectMode); }));

		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::CAMERA_LOCAL, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, calc_camera_local (meshTrans, camera), effectMode); }));

		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::NEAR_CLIP, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, static_cast<float>(camera->get_near_plane()), effectMode);  }));

		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::FAR_CLIP, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, static_cast<float>(camera->get_far_plane()), effectMode); }));

		_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type( ShaderSymantics::VIEWPORT, 
			[=](effect::Effect::ptr effect,	const std::string& paramName, scene::transform::Transform::ptr meshTrans, scene::transform::Camera::ptr camera, effect::Effect::EffectMode::type effectMode)
		{ effect->set_parameter(paramName, math::Vector4f(camera->get_viewport()->left, camera->get_viewport()->bottom, camera->get_viewport()->width(), camera->get_viewport()->height()), effectMode); }));

			//std::bind(&SemanticParamSetMap :: _fn_, this, std::placeholders::_1, _2, _3, _4, _5)));

		//INSERT_MAT_FN(ShaderSymantics::MODEL_MAT, model);
		//INSERT_MAT_FN(ShaderSymantics::MODEL_MAT_I, model_i);
		//INSERT_MAT_FN(ShaderSymantics::MODEL_MAT_IT, model_it);
// 		INSERT_MAT_FN(ShaderSymantics::VIEW_MAT, view);
// 		INSERT_MAT_FN(ShaderSymantics::VIEW_MAT_I, view_i);
// 		INSERT_MAT_FN(ShaderSymantics::VIEW_MAT_IT, view_it);
// 		INSERT_MAT_FN(ShaderSymantics::PROJECTION_MAT, proj);
// 		INSERT_MAT_FN(ShaderSymantics::PROJECTION_MAT_I, proj_i);
// 		INSERT_MAT_FN(ShaderSymantics::PROJECTION_MAT_IT, proj_it);
// 		INSERT_MAT_FN(ShaderSymantics::MODEL_VIEW_MAT, model_view);
// 		INSERT_MAT_FN(ShaderSymantics::MODEL_VIEW_MAT_I, model_view_i);
// 		INSERT_MAT_FN(ShaderSymantics::MODEL_VIEW_MAT_IT, model_view_it);
// 		INSERT_MAT_FN(ShaderSymantics::MODEL_VIEW_PROJECTION_MAT, model_view_proj);
// 		INSERT_MAT_FN(ShaderSymantics::MODEL_VIEW_PROJECTION_MAT_I, model_view_proj_i);
// 		INSERT_MAT_FN(ShaderSymantics::MODEL_VIEW_PROJECTION_MAT_IT, model_view_proj_it);
// 		INSERT_CAMERA_FN(ShaderSymantics::CAMERA_GLOBAL, camera_global);
// 		INSERT_CAMERA_FN(ShaderSymantics::CAMERA_LOCAL, camera_local);
// 		INSERT_CAMERA_FN(ShaderSymantics::NEAR_CLIP, near_clip);
// 		INSERT_CAMERA_FN(ShaderSymantics::FAR_CLIP, far_clip);
		//_semanticMatrixSetFunctionMap.insert(SemanticMatrixSetFunctionMap::value_type(MODEL_VIEW_PROJECTION_MAT, ));
	}

	//void model_view_proj(Material::ptr mat, const std::string& paramName, scene::transform::MeshTransform::ptr meshTrans, scene::transform::Camera::ptr camera)
	//{
	//	mat->setParameter(paramName, calc_model_view_proj(meshTrans, camera));
	//}

	//void model_view_proj_i(Material::ptr mat, const std::string& paramName, scene::transform::MeshTransform::ptr meshTrans, scene::transform::Camera::ptr camera)
	//{
	//	mat->setParameter(paramName, calc_model_view_proj(meshTrans, camera).inverse());
	//}

	//void model_view_proj_i(Material::ptr mat, const std::string& paramName, scene::transform::MeshTransform::ptr meshTrans, scene::transform::Camera::ptr camera)
	//{
	//	mat->setParameter(paramName, calc_model_view_proj(meshTrans, camera).inverse());
	//}

	static std::shared_ptr<SemanticParamSetMap> _globalInstance;

	SemanticMatrixSetFunctionMap _semanticMatrixSetFunctionMap;
};

}

#endif // __RENDER_SEMANTIC_PARAM_SETUP_HPP__