#ifndef _RENDER_FONTGL_HPP
#define _RENDER_FONTGL_HPP

#include <string>

#include "GLBase/texture.hpp"
#include "Scene/material.hpp"
#include "Scene/geometry.hpp"
#include "Math/rectangle.hpp"

namespace render {;

// font class ripped from a Nehe tutorial!
struct FontGL
{
private:
	glbase::Texture::ptr _data;
	//GLuint _base;				// Base Display List For The Font
	size_t _count;
	float _scale;

	int _charsX, _charsY;

	//scene::VertexSet::ptr _verts;
	//scene::TriangleSet::ptr _tris;
	scene::Material::ptr _material;

public:
	static const int MAX_CHARS;

	FontGL();
	~FontGL();

	void set_scale(float scale);
	void load(const std::string& filename, const std::string& shader);

	scene::Material::ptr get_material() const { return _material; }

	math::Rectanglef get_char_uvs(char ch) const;

	//const glbase::Texture& get_texture() const { return _data; }

	//int get_chars_x() const { return _charsX; }
	//int get_chars_y() const { return _charsY; }

	//void print(GLint x, GLint y, const std::string& str, unsigned int set = 0);
};

}

#endif // _RENDER_FONTGL_HPP