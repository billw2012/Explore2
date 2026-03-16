#ifndef texturearraymanager_h__
#define texturearraymanager_h__

#include <set>
#include <memory>
#include "GLBase/texture.hpp"

namespace explore2 {;

struct TextureArraySlot;
typedef std::shared_ptr<TextureArraySlot> TextureArraySlotHandle;

struct TextureArrayManager : public std::enable_shared_from_this<TextureArrayManager>
{
	friend struct TextureArraySlot;
	typedef std::shared_ptr<TextureArrayManager> ptr;

	void init1D(const std::string& name, GLuint wid, GLuint depth, GLint components, GLenum format, GLenum type, GLenum internalFormat, bool mipmaps = false);
	void init2D(const std::string& name, GLuint wid, GLuint height, GLuint depth, GLint components, GLenum format, GLenum type, GLenum internalFormat, bool mipmaps = false);

	TextureArraySlotHandle reserve(const GLvoid* data);

	glbase::Texture::ptr get_texture() const { return _texture; }

private:
	void free(GLuint slot);

private:
	bool _is2D; // true = 2d, false = 1d
	glbase::Texture::ptr _texture;
	std::set<GLuint> _freeSlots;
};


struct TextureArraySlot
{
	TextureArraySlot(const TextureArrayManager::ptr& manager, GLuint slot);
	~TextureArraySlot();

	void explicit_free();

	GLuint get_slot() const { return _slot; }

private:
	TextureArrayManager::ptr _manager;
	GLuint _slot;
};
}

#endif // texturearraymanager_h__
