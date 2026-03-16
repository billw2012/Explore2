#ifndef _SCENE_CAMERA_H
#define _SCENE_CAMERA_H


#include "group.hpp"
#include "matrixtraits.hpp"
#include "coordtraits.hpp"
#include "GLBase/texture.hpp"
#include "fbo/framebufferObject.h"
#include "Math/transformation.hpp"
#include "Math/ray.hpp"
#include "Math/matrix4.hpp"
#include "Math/rectangle.hpp"
#include "viewport.hpp"
#include "Math/frustum.hpp"

namespace scene {;
namespace transform {;

struct ProjectionType
{
	enum type
	{
		Perspective,
		Orthographic
	};
};

struct Camera : public Group
{
	typedef std::shared_ptr< Camera > ptr;
	typedef std::set<glbase::Texture::ptr> TextureSet;

	typedef math::Rectangle<Transform::float_type> rect_type;
private:
	ProjectionType::type _type;

	mutable matrix4_type _projection;

	Transform::float_type _nearPlane, _farPlane;
	double _fov;

	Viewport::ptr _viewport;
	rect_type _renderArea;

	mutable bool _needsRecalc;

public:
	Camera(const std::string& name = "unnamedCamera") : Group(name), _type(ProjectionType::Perspective), _projection(), 
		_nearPlane(1), _farPlane(10000), /*_viewport(new Viewport(0, 0, 1024, 768)),*/ _fov(60), _needsRecalc(true) /*,_renderArea(float_type(0), float_type(0), float_type(1024), float_type(768))*/ {}

	virtual Transform* clone() const
	{
		Camera* newCamera = new Camera(*this);
		clone_data(newCamera);
		return newCamera;
	}

	const matrix4_type& projection() const
	{
		if(_needsRecalc)
			recalc_projection_matrix();
		return _projection;
	}

	void set_type(ProjectionType::type type_) { _type = type_; _needsRecalc = true; }
	ProjectionType::type get_type() const { return _type; }
	void set_near_plane(Transform::float_type nearPlane) { _nearPlane = nearPlane; _needsRecalc = true; }
	Transform::float_type get_near_plane() const { return _nearPlane; }
	void set_far_plane(Transform::float_type farPlane) { _farPlane = farPlane; _needsRecalc = true; }
	Transform::float_type get_far_plane() const { return _farPlane; }

	void set_viewport(const Viewport::ptr& viewport) { _viewport = viewport; _needsRecalc = true;	}
	const Viewport::ptr& get_viewport() const { return _viewport; }

	void set_render_area(const rect_type& renderArea) { _renderArea = renderArea; _needsRecalc = true; }

	void set_projection(const matrix4_type& projection) 
	{ 
		_projection = projection;
		_needsRecalc = false;
	}

	//void setRenderPos(screen_metric_type x, screen_metric_type y) { _renderX = x; _renderY = y; }
	//void setRenderSize(screen_metric_type width, screen_metric_type height) { _renderWidth = width; _renderHeight = height; _needsRecalc = true; }
	
	const rect_type& get_render_area() const { return _renderArea; }

	//screen_metric_type getRenderX() const { return _renderX; }
	//screen_metric_type getRenderY() const { return _renderY; }

	//screen_metric_type getRenderWidth() const { return _renderWidth; }
	//screen_metric_type getRenderHeight() const { return _renderHeight; }

	void setFOV(double fov) { _fov = fov; _needsRecalc = true; }
	double getFOV() const { return _fov; }
	double getFOVH() const 
	{
		// calculate horizontal fov using ratio between render width and height, and trig
// 		soh cah toa
// 			tan(r) = opp/adj
// 			tan(r)*adj = opp
// 			adj = opp/tan(r)
		// calculate adj
		double a = (double(_viewport->height())*0.5f) / std::tan(math::deg_to_rad(_fov) * 0.5f);
		// calculate vertical fov
		return math::rad_to_deg(atan2(double(_viewport->width())*0.5, a)) * 2.0f; 
	}

	virtual NodeType::type get_node_type() const
	{
		return NodeType::Camera;
	}

	math::Vector3d unproject_global(const math::Vector3d& pt) const
	{
		math::Vector4d inp((pt.x - (double)_viewport->left - 0.5 * (double)_viewport->width()) / (0.5 * (double)_viewport->width()), 
			(pt.y - (double)_viewport->bottom - 0.5 * (double)_viewport->height()) / (0.5 * (double)_viewport->height()), pt.z, 1.0);
		math::Vector4d pp((projection() * globalTransformInverse()).inverse() * Transform::vec4_type(inp));
		if(pp.w == 0.0)
			return math::Vector3d();
		pp.x /= pp.w;
		pp.y /= pp.w;
		pp.z /= pp.w;
		return math::Vector3d(pp);
	}

	math::Vector3d project_global(const math::Vector3d& pt) const
	{
		math::Vector4d ppt(projection() * (globalTransformInverse() * Transform::vec4_type(pt, 1.0)));
		return math::Vector3d(0.5 * (double)_viewport->width() * (ppt.x / ppt.w) + (double)_viewport->left + 0.5 * (double)_viewport->width(),
			0.5 * (double)_viewport->height() * (ppt.y / ppt.w) + (double)_viewport->bottom + 0.5 * (double)_viewport->height(),
			ppt.z / ppt.w);
	}

	math::Vector3d project_local(const math::Vector3d& pt) const
	{
		math::Vector4d ppt(projection() * Transform::vec4_type(pt, 1.0));
		return math::Vector3d(0.5 * (double)_viewport->width() * (ppt.x / ppt.w) + (double)_viewport->left + 0.5 * (double)_viewport->width(),
			0.5 * (double)_viewport->height() * (ppt.y / ppt.w) + (double)_viewport->bottom + 0.5 * (double)_viewport->height(),
			ppt.z / ppt.w);
	}

	math::Rayd project_ray(int x, int y) const
	{
		using namespace math;

		Vector3d p0(unproject_global(Vector3d(static_cast<double>(x), static_cast<double>(y), 0.0)));
		Vector3d p1(unproject_global(Vector3d(static_cast<double>(x), static_cast<double>(y), 1.0)));
	
		return Rayd(p0, (p1-p0).normal());
	}

	math::Frustumd get_global_frustum() const 
	{
		//return math::Frustumd(forwardGlobal(), rightGlobal(), upGlobal(), centerGlobal(), 
		//	getFOV(), getFOVH(), get_near_plane(), get_far_plane());
		return math::Frustumd(projection() * globalTransformInverse());
	}

	math::Frustumd get_local_frustum() const 
	{
		//return math::Frustumd(getFOV(), getFOVH(), get_near_plane(), get_far_plane());
		return math::Frustumd(projection());
	}

private:
	void recalc_projection_matrix() const
	{
		static const double _0 = static_cast<double>(0);
		static const double _0_5 = static_cast<double>(0.5);
		static const double _1 = static_cast<double>(1);
		static const double _2 = static_cast<double>(2);
		if(_type == ProjectionType::Perspective)
		{
			_projection = Transform::matrix4_type(math::perspective(Transform::float_type(_fov), Transform::float_type(_viewport->width()) / Transform::float_type(_viewport->height()), _nearPlane, _farPlane));
		}
		else
		{
			_projection = Transform::matrix4_type(math::ortho(_renderArea.left, _renderArea.right, _renderArea.bottom, _renderArea.top, _nearPlane, _farPlane));
		}
		_needsRecalc = false;
	}
};

}

}

#endif // _SCENE_CAMERA_H
