#include "texture.hpp"
#include "sdlgl.hpp"

#include <unordered_set>

namespace glbase {;

namespace {;

std::vector<Texture*> _textures;

void register_texture(Texture* tex)
{
	_textures.push_back(tex);
}

void unregister_texture(Texture* tex)
{
	auto fItr = std::find(_textures.begin(), _textures.end(), tex);
	if(fItr != _textures.end())
		_textures.erase(fItr);
}

}

Texture::Texture(const std::string& name /*= std::string()*/) 
	: _handle(0),
	_valid(false),
	_width(0),
	_height(0),
	_depth(0),
	_components(0),
	_format(0),
	_internalformat(0),
	_type(0),
	_target(0),
	_mipmapped(false),
	_minFilter(FilterMode::None), 
	_magFilter(FilterMode::None),
	_name(name)
{
}

Texture::Texture(const char* name) 
	: _handle(0),
	_valid(false),
	_width(0),
	_height(0),
	_depth(0),
	_components(0),
	_format(0),
	_internalformat(0),
	_type(0),
	_target(0),
	_mipmapped(false),
	_minFilter(FilterMode::None), 
	_magFilter(FilterMode::None),
	_name(name)
{
}

Texture::Texture(const boost::filesystem::path& fileName, LoadOptions::type loadOptions)
	: _handle(0),
	_valid(false),
	_width(0),
	_height(0),
	_depth(0),
	_components(0),
	_format(0),
	_internalformat(0),
	_type(0),
	_target(0),
	_mipmapped(false),
	_minFilter(FilterMode::None), 
	_magFilter(FilterMode::None)
{
	if(!fileName.empty())
		load(fileName, loadOptions);
}


void Texture::create1D(GLenum target, GLuint width, 
							  GLint components, GLenum format, GLenum type, 
							  const GLvoid *data, GLenum internalFormat, bool mipmaps) 
{
	create3D(target, width, 0, 0, components, format, type, data, internalFormat, mipmaps);
}

void Texture::create2D(GLenum target, GLuint width, GLuint height, 
							GLint components, GLenum format, GLenum type, 
							const GLvoid *data, GLenum internalFormat, bool mipmaps) 
{
	create3D(target, width, height, 0, components, format, type, data, internalFormat, mipmaps);
}

void Texture::create3D(GLenum target, GLuint width, GLuint height, GLuint depth,
							  GLint components, GLenum format, GLenum type, 
							  const GLvoid *data, GLenum internalFormat, bool mipmaps) 
{
	destroy();
	_target = target;
	glGenTextures(1, &_handle);
	register_texture(this);
	push_bind(_target, _handle);
	pre_setup_texture();
	CHECK_OPENGL_ERRORS;
	assert(internalFormat > 4);
	if(target == GL_TEXTURE_3D || target == GL_TEXTURE_2D_ARRAY)
	{
		glTexImage3D(_target, 0, internalFormat, width, height, depth, 0, format, type, data);
	}
	else if(target == GL_TEXTURE_2D || target == GL_TEXTURE_RECTANGLE || target == GL_TEXTURE_1D_ARRAY)
	{
		glTexImage2D(_target, 0, internalFormat, width, height, 0, format, type, data);
	}
	else if(target == GL_TEXTURE_1D)
	{
		glTexImage1D(_target, 0, internalFormat, width, 0, format, type, data);
	}
	CHECK_OPENGL_ERRORS;

	_width = width;
	_height = height;
	_components = components;
	_format = format;
	_internalformat = internalFormat;
	_type = type;
	_target = target;
	_valid = true;

	setup_texture();
	CHECK_OPENGL_ERRORS;

	pop_bind(_target);
}

void Texture::load_data_2D(GLint level, GLuint xoffset, GLuint yoffset, GLsizei width, GLsizei height, GLenum format, GLenum type, const GLvoid * data)
{
	push_bind(_target, _handle);

	glTexSubImage2D(_target, level, xoffset, yoffset, width, height, format, type, data);
	CHECK_OPENGL_ERRORS;

	pop_bind(_target);
}

void Texture::load_data_3D(GLint level, GLuint xoffset, GLuint yoffset, GLuint zoffset, GLsizei width, GLsizei height, GLsizei depth, GLenum format, GLenum type, const GLvoid * data)
{
	push_bind(_target, _handle);

	glTexSubImage3D(_target, level, xoffset, yoffset, zoffset, width, height, depth, format, type, data);
	CHECK_OPENGL_ERRORS;

	pop_bind(_target);
}

// void Texture::setCubeFace(GLenum face, GLvoid* data)
// {
// 	assert(_target == GL_TEXTURE_CUBE_MAP);
// 	assert(_internalformat > 4);
// 	glTexImage2D(face, 0, _internalformat, _width, _height, 0, _format, _type, data);
// }

bool Texture::load_cube(const std::string &fileStart, const std::string &fileExt, bool mipmaps)
{
	destroy();

	glGenTextures(1, &_handle);
	register_texture(this);
	ILint format; 
	ILint type;
	GLenum internalformat;
	_target = GL_TEXTURE_CUBE_MAP;
	push_bind(_target, _handle);
	pre_setup_texture();
	for(unsigned short face = 0; face < 6; ++face)
	{
		ILuint ilhandle;
		ilGenImages(1, &ilhandle);
		ilBindImage(ilhandle);
		std::stringstream fileName;
		fileName << fileStart << face << '.' << fileExt;
		ilLoadImage(fileName.str().c_str());
		throw_on_IL_error();

		if(face == 0)
		{
			_width = ilGetInteger(IL_IMAGE_WIDTH);
			_height = ilGetInteger(IL_IMAGE_HEIGHT);
			if(!isPOT(_width) || !isPOT(_height) || _width != _height)
				throw std::exception("Image is a cube map but either width != height, or they are not both powers of 2.");
			_depth = ilGetInteger(IL_IMAGE_DEPTH);
			_components = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
			format = ilGetInteger(IL_IMAGE_FORMAT); 
			_format = formatILtoGL(format);
			type = ilGetInteger(IL_IMAGE_TYPE);
			_type = typeILtoGL(type);
			internalformat = internalILtoGL(format, type, _components);
		}
		else
		{
			if(_width != ilGetInteger(IL_IMAGE_WIDTH))
				throw std::exception("Image widths to not match.");
			if(_height != ilGetInteger(IL_IMAGE_HEIGHT))
				throw std::exception("Image widths to not match.");
			if(_depth != ilGetInteger(IL_IMAGE_DEPTH))
				throw std::exception("Image widths to not match.");
			if(_components != ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL))
				throw std::exception("Image widths to not match.");
			if(format != ilGetInteger(IL_IMAGE_FORMAT))
				throw std::exception("Face formats do not match.");
			if(type != ilGetInteger(IL_IMAGE_TYPE))
				throw std::exception("Face types do not match.");
		}
		if(ilGetInteger(IL_NUM_IMAGES) != 0)
			throw std::exception("Face contains more than a single image.");
		if(ilGetInteger(IL_IMAGE_CUBEFLAGS) != 0)
			throw std::exception("Yo dawg I heard you like cube-maps so you tried to put a cube-map inside your cube-map and crashed this program.");

		ilBindImage(ilhandle);
		glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+face, 0, internalformat, _width, _height, 0, _format, _type, ilGetData());
		ilDeleteImage(ilhandle);
	}
	setup_texture();
	_valid = true;
	pop_bind(_target);

	return _valid;
}

void Texture::load(const boost::filesystem::path& fileName, LoadOptions::type loadOptions)
{
	destroy();

	ILuint ilhandle;
	register_texture(this);
	ilGenImages(1, &ilhandle);
	ilBindImage(ilhandle);
	ilLoadImage(fileName.string().c_str());
	throw_on_IL_error();

	_width = ilGetInteger(IL_IMAGE_WIDTH);
	_height = ilGetInteger(IL_IMAGE_HEIGHT);
	_depth = ilGetInteger(IL_IMAGE_DEPTH);
	_components = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
	ILint format = ilGetInteger(IL_IMAGE_FORMAT); 
	ILint type = ilGetInteger(IL_IMAGE_TYPE);
	ILint images = ilGetInteger(IL_NUM_IMAGES);
	ILenum cubeFlags = ilGetInteger(IL_IMAGE_CUBEFLAGS);

	_format = formatILtoGL(format);
	_type = typeILtoGL(type);
	GLenum internalformat = internalILtoGL(format, type, _components);

	unsigned char *data = ilGetData();

	// determine if image is pot or rect, if it is pot determine if it is a cube map or a 3d map
	glGenTextures(1, &_handle);
	// if its a cube map
	if(cubeFlags != 0)
	{
		if(!isPOT(_width) || !isPOT(_height) || _width != _height)
			throw std::exception("Image is a cube map but either width != height, or they are not both powers of 2.");
		_target = GL_TEXTURE_CUBE_MAP;
		push_bind(_target, _handle);
		pre_setup_texture();
		for(int i=0; i<6; i++)
		{
			ilBindImage(ilhandle);
			ilActiveFace(i);
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, internalformat, _width, _height, 0, _format, _type, ilGetData());
		}
		ilBindImage(ilhandle);
		ilActiveFace(0);
		setup_texture();
		pop_bind(_target);
	}
	else if(images == 0)
	{
		loadTexture2D(loadOptions, data, internalformat);
	}

	ilDeleteImage(ilhandle);

	_fileName = fileName;
	_name = _fileName.string();
	_valid = true;
}

void Texture::destroy()
{
	if(_valid)
	{
		glDeleteTextures(1, &_handle);
		unregister_texture(this);
	}
	_valid = false;
}

void Texture::set_wrap(WrapMode::type smode, WrapMode::type tmode, WrapMode::type rmode)
{
	push_bind(_target, _handle);
	setWrapInternal(smode, tmode, rmode);
	pop_bind(_target);
}

void Texture::setWrapInternal(WrapMode::type smode, WrapMode::type tmode, WrapMode::type rmode)
{
	glTexParameteri(_target, GL_TEXTURE_WRAP_S, wrapModeToGL(smode));
	if(tmode != WrapMode::None)
		glTexParameteri(_target, GL_TEXTURE_WRAP_T, wrapModeToGL(tmode));
	if(rmode != WrapMode::None)
		glTexParameteri(_target, GL_TEXTURE_WRAP_R, wrapModeToGL(rmode));
}

void Texture::set_filter(FilterMode::type minFilter, FilterMode::type magFilter)
{
	push_bind(_target, _handle);
	set_filter_internal(minFilter, magFilter);
	pop_bind(_target);
}

void Texture::set_filter_internal(FilterMode::type minFilter, FilterMode::type magFilter)
{
	_minFilter = minFilter;
	_magFilter = magFilter;
	switch(magFilter)
	{
	case FilterMode::Linear:
		glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		break;
	default:
		glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	};

	glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, filterModeToGL(minFilter));
}

void Texture::generate_mipmaps()
{
	push_bind(_target, _handle);
	glGenerateMipmap(_target);
	pop_bind(_target);
	_mipmapped = true;
	set_filter(_minFilter, _magFilter);
}

void Texture::setup_texture()
{
	if (_target == GL_TEXTURE_2D || _target == GL_TEXTURE_2D_ARRAY) 
	{
		set_filter_internal(FilterMode::Linear, FilterMode::Linear);
		//_mipmapped = true;
		//glGenerateMipmap(_target);
		//setWrapInternal(WrapMode::Repeat, WrapMode::Repeat, WrapMode::None);
		//setFilter(FilterMode::Linear, FilterMode::Linear);
	}	
	else if (_target == GL_TEXTURE_3D) 
	{
		set_filter_internal(FilterMode::Linear, FilterMode::Linear);
		//_mipmapped = true;
		//glGenerateMipmap(_target);
		//setWrapInternal(WrapMode::Repeat, WrapMode::Repeat, WrapMode::Repeat);
		//setFilter(FilterMode::Linear, FilterMode::Linear);
	}
	else if (_target == GL_TEXTURE_CUBE_MAP) 
	{
		set_filter_internal(FilterMode::Linear, FilterMode::Linear);
		//_mipmapped = true;
		//glGenerateMipmap(_target);
		//setWrapInternal(WrapMode::ClampToEdge, WrapMode::ClampToEdge, WrapMode::ClampToEdge);
		//setFilter(FilterMode::Linear, FilterMode::Linear);
	}
	else if(_target == GL_TEXTURE_RECTANGLE)
	{
		set_filter_internal(FilterMode::Nearest, FilterMode::Nearest);
		//_mipmapped = false;
		//setWrapInternal(WrapMode::ClampToEdge, WrapMode::ClampToEdge, WrapMode::None);
		//setFilter(FilterMode::Nearest, FilterMode::Nearest);
	}
	else if (_target == GL_TEXTURE_1D || _target == GL_TEXTURE_1D_ARRAY) 
	{
		set_filter_internal(FilterMode::Linear, FilterMode::Linear);
		//_mipmapped = true;
		//glGenerateMipmap(_target);
		//setWrapInternal(WrapMode::Repeat, WrapMode::None, WrapMode::None);
		//setFilter(FilterMode::Linear, FilterMode::Linear);
	}
	//else if(_target == GL_TEXTURE_2D_ARRAY || _target == GL_TEXTURE_1D_ARRAY)
	//{
	//	set_filter_internal(FilterMode::Linear, FilterMode::Linear);
	//	//_mipmapped = true;
	//	//glGenerateMipmap(_target);
	//	//glTexParameteri( _target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	//	//glTexParameteri( _target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	//	//setWrapInternal(WrapMode::ClampToEdge, WrapMode::ClampToEdge, WrapMode::None);	
	//	//setFilter(FilterMode::Linear, FilterMode::Linear);
	//}
	////glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
	//glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	//glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	//glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	//glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	//glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	//glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	//glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
}

void Texture::set_shadowing_parameters()
{
	push_bind(_target, _handle);
	glTexParameteri( _target, GL_TEXTURE_COMPARE_MODE, GL_COMPARE_R_TO_TEXTURE);
	glTexParameteri( _target, GL_TEXTURE_COMPARE_FUNC, GL_LEQUAL);
	setWrapInternal(WrapMode::ClampToEdge, WrapMode::ClampToEdge, WrapMode::None);	
	pop_bind(_target);
}

void Texture::pre_setup_texture()
{
	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
	glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
	glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
	glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
}

GLenum Texture::wrapModeToGL(WrapMode::type mode)
{
	switch(mode)
	{
	case WrapMode::ClampToEdge: return GL_CLAMP_TO_EDGE;
	case WrapMode::MirroredRepeat: return GL_MIRRORED_REPEAT;
		//case WrapMode::Repeat: return GL_REPEAT;
	default: return GL_REPEAT;
	};
}

GLenum Texture::filterModeToGL(FilterMode::type mode)
{
	switch(mode)
	{
	case FilterMode::Linear: return (_mipmapped?  GL_LINEAR_MIPMAP_LINEAR : GL_LINEAR);
	default: return (_mipmapped? GL_NEAREST_MIPMAP_NEAREST : GL_NEAREST);
	};
}

GLenum Texture::internalILtoGL(int ilFormat, int ilType, int components)
{
	if(ilFormat == IL_BGR || ilFormat == IL_RGB || components == 3)
		return GL_RGB8;
	if(ilFormat == IL_BGRA || ilFormat == IL_RGBA || components == 4)
		return GL_RGBA8;
	if(components == 1)
		return GL_LUMINANCE8;
	assert(false);
	return GL_RGB8;
}

GLenum Texture::formatILtoGL(int ilFormat)
{
	switch(ilFormat)
	{
	case IL_BGR:
		return GL_BGR;
	case IL_BGRA:
		return GL_BGRA;
	case IL_COLOUR_INDEX:
		return GL_COLOR_INDEX;
	case IL_LUMINANCE:
		return GL_LUMINANCE;
	case IL_LUMINANCE_ALPHA:
		return GL_LUMINANCE_ALPHA;
	case IL_RGB:
		return GL_RGB;
	case IL_RGBA:
		return GL_RGBA;
	default:
		return -1;
	};		
}

int Texture::formatGLtoIL(GLenum fmt)
{
	switch(fmt)
	{
	case GL_BGR:
		return IL_BGR;
	case GL_BGRA:
		return IL_BGRA;
	case GL_COLOR_INDEX:
		return IL_COLOUR_INDEX;
	case GL_LUMINANCE:
		return IL_LUMINANCE;
	case GL_LUMINANCE_ALPHA:
		return IL_LUMINANCE_ALPHA;
	case GL_RGB:
		return IL_RGB;
	case GL_RGBA:
		return IL_RGBA;
	default:
		return -1;
	};		
}

GLenum Texture::typeILtoGL(int ilType)
{
	switch(ilType)
	{
	case IL_BYTE:
		return GL_BYTE;
	case IL_SHORT:
		return GL_SHORT;
	case IL_INT:
		return GL_INT;
	case IL_UNSIGNED_BYTE:
		return GL_UNSIGNED_BYTE;
	case IL_UNSIGNED_SHORT:
		return GL_UNSIGNED_SHORT;
	case IL_UNSIGNED_INT:
		return GL_UNSIGNED_INT;
	case IL_FLOAT:
		return GL_FLOAT;
	case IL_DOUBLE:
		return GL_DOUBLE;
	default:
		return -1;
	};		
}

int Texture::typeGLtoIL(GLenum type)
{
	switch(type)
	{
	case GL_BYTE:
		return IL_BYTE;
	case GL_SHORT:
		return IL_SHORT;
	case GL_INT:
		return IL_INT;
	case GL_UNSIGNED_BYTE:
		return IL_UNSIGNED_BYTE;
	case GL_UNSIGNED_SHORT:
		return IL_UNSIGNED_SHORT;
	case GL_UNSIGNED_INT:
		return IL_UNSIGNED_INT;
	case GL_FLOAT:
		return IL_FLOAT;
	case GL_DOUBLE:
		return IL_DOUBLE;
	default:
		return -1;
	};		
}

bool Texture::isPOT(unsigned int val)
{
	while(val != 1)
	{
		if((val & 1) != 0)
			return false;
		val >>= 1;
	}
	return true;
}

void Texture::loadTexture2D( LoadOptions::type loadOptions, unsigned char * data, GLenum internalformat )
{
	if(isPOT(_width) && isPOT(_height) && !(loadOptions & LoadOptions::AsRect))
	{
		_target = GL_TEXTURE_2D;
	}
	else
	{
		_target = GL_TEXTURE_RECTANGLE;
	}

	push_bind(_target, _handle);
	glTexImage2D(_target, 0, internalformat, _width, _height, 0, _format, _type, data);
	setup_texture();
	pop_bind(_target);
}

GLint Texture::_sOldBoundTexture = 0;

namespace {;

GLenum get_query_target(GLenum target)
{
	switch(target)
	{
	case GL_TEXTURE_1D:	return GL_TEXTURE_BINDING_1D; 
	case GL_TEXTURE_1D_ARRAY:	return GL_TEXTURE_BINDING_1D_ARRAY; 
	case GL_TEXTURE_2D:	return GL_TEXTURE_BINDING_2D; 
	case GL_TEXTURE_2D_ARRAY:	return GL_TEXTURE_BINDING_2D_ARRAY; 
	case GL_TEXTURE_2D_MULTISAMPLE:	return GL_TEXTURE_BINDING_2D_MULTISAMPLE; 
	case GL_TEXTURE_2D_MULTISAMPLE_ARRAY:	return GL_TEXTURE_BINDING_2D_MULTISAMPLE_ARRAY; 
	case GL_TEXTURE_3D:	return GL_TEXTURE_BINDING_3D; 
	case GL_TEXTURE_BUFFER:	return GL_TEXTURE_BINDING_BUFFER; 
	case GL_TEXTURE_CUBE_MAP:	return GL_TEXTURE_BINDING_CUBE_MAP; 
	case GL_TEXTURE_RECTANGLE:	return GL_TEXTURE_BINDING_RECTANGLE; 
	};
	return GL_TEXTURE_BINDING_2D;
}

};

void Texture::push_bind(GLenum target, GLuint handle)
{
	assert(_sOldBoundTexture == 0);
	glGetIntegerv(get_query_target(target), &_sOldBoundTexture);
	glBindTexture(target, handle);
}

void Texture::pop_bind(GLenum target)
{
	glBindTexture(target, _sOldBoundTexture);
	_sOldBoundTexture = 0;
}

void Texture::get_simple( SimpleTextureData& data, GLuint level ) const
{
	push_bind(_target, _handle);

	glGetTexLevelParameteriv(_target, 0, GL_TEXTURE_WIDTH, &data.width);
	glGetTexLevelParameteriv(_target, 0, GL_TEXTURE_HEIGHT, &data.height);
	glGetTexLevelParameteriv(_target, 0, GL_TEXTURE_DEPTH, &data.depth);
	data.pixels.resize(data.width * data.height * data.depth * 4);

	glGetTexImage(_target, 0, GL_RGBA, GL_FLOAT, (GLvoid*)&data.pixels[0]);
	//if(_target == GL_TEXTURE_2D)
	//{
	//	//GLint width, height;
	//	glGetTexImage(_target, 0, GL_RGBA, GL_FLOAT, (GLvoid*)&data.pixels[0]);
	//}

	pop_bind(_target);
}

int TextureManager::get_count()
{
	return (int)_textures.size();
}

Texture* TextureManager::get( int idx )
{
	return _textures[idx];
}

// void Texture::save( const std::string& fileName )
// {
// 	ilutGLSaveImage(const_cast<char*>(fileName.c_str()), _handle);
// }


};