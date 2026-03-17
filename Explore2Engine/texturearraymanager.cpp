
#include "texturearraymanager.h"

namespace explore2 {;



void TextureArrayManager::init1D( const std::string& name, GLuint wid, GLuint depth, GLint components, GLenum format, GLenum type, GLenum internalFormat, bool mipmaps )
{
	_texture = glbase::Texture::ptr(new glbase::Texture(name.c_str()));

	_texture->create2D(GL_TEXTURE_1D_ARRAY, wid, depth, components, format, type, NULL, internalFormat, mipmaps);

	for(GLuint idx = 0; idx < depth; ++idx)
		_freeSlots.insert(idx);

	_is2D = false;
}

void TextureArrayManager::init2D( const std::string& name, GLuint wid, GLuint height, GLuint depth, GLint components, GLenum format, GLenum type, GLenum internalFormat, bool mipmaps )
{
	_texture = glbase::Texture::ptr(new glbase::Texture(name.c_str()));

	_texture->create3D(GL_TEXTURE_2D_ARRAY, wid, height, depth, components, format, type, NULL, internalFormat, mipmaps);

	for(GLuint idx = 0; idx < depth; ++idx)
		_freeSlots.insert(idx);

	_is2D = true;
}

TextureArraySlotHandle TextureArrayManager::reserve( const GLvoid* data )
{
	if(_freeSlots.empty())
		return TextureArraySlotHandle();

	GLuint slot = *_freeSlots.begin();
	_freeSlots.erase(_freeSlots.begin());

	if(_is2D)
	{
		// as its an array we actually use the 3D load data function
		_texture->load_data_3D(0, 0, 0, slot, _texture->width(), _texture->height(), 1, _texture->format(), _texture->type(), data);
	}
	else
	{
		// as its an array we actually use the 2D load data function
		_texture->load_data_2D(0, 0, slot, _texture->width(), 1, _texture->format(), _texture->type(), data);
	}

	return TextureArraySlotHandle(new TextureArraySlot(shared_from_this(), slot));
}

void TextureArrayManager::free( GLuint slot )
{
	_freeSlots.insert(slot);
}


TextureArraySlot::TextureArraySlot( const TextureArrayManager::ptr& manager, GLuint slot )
	: _manager(manager),
	_slot(slot)
{

}

TextureArraySlot::~TextureArraySlot()
{
	if(_manager != NULL)
	{
		_manager->free(_slot);
	}
}

void TextureArraySlot::explicit_free()
{
	if(_manager != NULL)
	{
		_manager->free(_slot);
		_manager = NULL;
	}	
}

}