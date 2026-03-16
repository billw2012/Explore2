
#include "gbuffer.h"

namespace explore2 {;

void GBuffer::init(int width, int height)
{
	using namespace glbase;

	_fbo = FramebufferObject::ptr(new FramebufferObject());

	_albedo = Texture::ptr(new Texture("albedo"));
	_albedo->create2D(GL_TEXTURE_RECTANGLE, width, height, 4, GL_RGBA, GL_FLOAT, NULL, GL_RGBA32F);
	_fbo->AttachTexture(_albedo->handle(), GL_COLOR_ATTACHMENT0_EXT);

	_normal = Texture::ptr(new Texture("normal"));
	_normal->create2D(GL_TEXTURE_RECTANGLE, width, height, 4, GL_RGBA, GL_FLOAT, NULL, GL_RGBA32F);
	_fbo->AttachTexture(_normal->handle(), GL_COLOR_ATTACHMENT1_EXT);

	_specular = Texture::ptr(new Texture("specular"));
	_specular->create2D(GL_TEXTURE_RECTANGLE, width, height, 4, GL_RGBA, GL_FLOAT, NULL, GL_RGBA32F);
	_fbo->AttachTexture(_specular->handle(), GL_COLOR_ATTACHMENT2_EXT);

	_position = Texture::ptr(new Texture("position"));
	_position->create2D(GL_TEXTURE_RECTANGLE, width, height, 4, GL_RGBA, GL_FLOAT, NULL, GL_RGBA32F);
	_fbo->AttachTexture(_position->handle(), GL_COLOR_ATTACHMENT3_EXT);

	_depth = Texture::ptr(new Texture("depth"));
	_depth->create2D(GL_TEXTURE_RECTANGLE, width, height, 1, GL_DEPTH_COMPONENT, GL_FLOAT, NULL, GL_DEPTH_COMPONENT32F);
	_fbo->AttachTexture(_depth->handle(), GL_DEPTH_ATTACHMENT);

	assert(_fbo->IsValid(std::cout));
}

FramebufferObject::ptr GBuffer::get_fbo() const
{
	return _fbo;
}

glbase::Texture::ptr GBuffer::get_albedo() const
{
	return _albedo;
}

glbase::Texture::ptr GBuffer::get_normal() const
{
	return _normal;
}

glbase::Texture::ptr GBuffer::get_specular() const
{
	return _specular;
}

glbase::Texture::ptr GBuffer::get_position() const
{
	return _position;
}

glbase::Texture::ptr GBuffer::get_depth() const
{
	return _depth;
}


}