
#if !defined(__EXPLORE2_IMPOSTERABLE_H__)
#define __EXPLORE2_IMPOSTERABLE_H__

#include "Render/geometryset.h"

namespace render {;

struct Impostorable
{
	virtual GeometrySet::ptr get_impostor_geometry() const = 0;
	virtual void activate() const = 0;
	virtual void deactivate() const = 0;
};

struct ImposterGenerator
{
	void init(RenderGL* renderer)
	void update_impostor(Impostorable* impostorable, scene::transform::Camera::ptr camera);

private:
	RenderGL* _renderer;
};

}

#endif // __EXPLORE2_IMPOSTERABLE_H__