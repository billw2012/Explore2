#if !defined(__GUI_RENDERSTAGE_HPP__)
#define __GUI_RENDERSTAGE_HPP__

#include "gui_common/object.hpp"
#include "fbo/framebufferObject.h"

namespace render
{

template < 
	class MeshTransformTy_
>
struct GUIRenderStage
{
	typedef math::Rectangle< unsigned int > ViewportType;
	typedef MeshTransformTy_ MeshTransformType;
	typedef gui::Object<MeshTransformType> ObjectType;

private:
	typedef GUIRenderStage<MeshTransformTy_> this_type;
public:
	typedef boost::shared_ptr< this_type > ptr;

	GUIRenderStage() : _priority(0) {}

	void set_base_component(typename ObjectType::ptr component) { _baseComponent = component; }

	typename ObjectType::ptr get_base_component() const { return _baseComponent; }

	void set_viewport(const ViewportType& viewport) { _viewport = viewport; }
	const ViewportType& get_viewport() const { return _viewport; }

	unsigned int get_priority() const { return _priority; }
	void set_priority(unsigned int priority) { _priority = priority; }

	void set_fbo_target(FramebufferObject::ptr fboTarget) { _fboTarget = fboTarget; }
	FramebufferObject::ptr get_fbo_target() const { return _fboTarget; }

private:
	ViewportType _viewport;
	typename ObjectType::ptr _baseComponent;
	unsigned int _priority;
	FramebufferObject::ptr _fboTarget;
};

}				
#endif // __GUI_RENDERSTAGE_HPP__
