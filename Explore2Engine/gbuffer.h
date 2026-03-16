#ifndef gbuffer_h__
#define gbuffer_h__

#include "GLBase/texture.hpp"
#include "fbo/framebufferObject.h"

namespace explore2 {;

struct GBuffer
{
	typedef std::shared_ptr<GBuffer> ptr;

	void init(int width, int height);

	FramebufferObject::ptr get_fbo() const;

	glbase::Texture::ptr get_albedo() const;
	glbase::Texture::ptr get_normal() const;
	glbase::Texture::ptr get_specular() const;
	glbase::Texture::ptr get_position() const;
	glbase::Texture::ptr get_depth() const;

private:
	FramebufferObject::ptr _fbo;
	glbase::Texture::ptr _albedo, _normal, _specular, _position, _depth;
};

}

#endif // gbuffer_h__
