#pragma once

// stl
#include <stdexcept>
#include <map>

// boost
#include <boost/shared_ptr.hpp>
#include <boost/shared_array.hpp>

// misc
#include <IL/il.h>
#include <IL/ilut.h>

#ifdef WIN32
#pragma comment(lib, "DevIL.lib")
#pragma comment(lib, "ILU.lib")
#pragma comment(lib, "ILUT.lib")
#endif

namespace render
{

namespace TextureType 
{ 
enum type
{
	BYTE,
	FLOAT
};
}

//////////////////////////////////////////////////////////////////////////
// Texture class
// For holding texture data.
struct Texture
{
	// fields
private:
	GLuint _handle;
	std::string _name;
	bool _valid;

	GLuint _width;
	GLuint _height;
	GLuint _depth;
	GLubyte _components;
	GLenum _format;
	GLenum _type;
	GLenum _target;

	// methods
public:
	Texture() 
		: _handle(0),
		_name(),
		_valid(false),
		_width(0),
		_height(0),
		_depth(0),
		_components(0),
		_format(0),
		_type(0),
		_target(0) {}
	~Texture()
	{
		destroy();
	}

	void create(GLenum target, GLuint width, GLuint height, GLint components,
		GLenum format, GLenum type, GLvoid *data, GLenum internalFormat, bool mipmaps, const std::string &name) 
	{
		destroy();

		glGenTextures(1, &_handle);
		glBindTexture(_target, _handle);
		if(mipmaps)
			gluBuild2DMipmaps(_target, components, width, height, format, type, data);
		else
			glTexImage2D(_target, 0, internalFormat, width, height, 0, format, type, data);

		_width = width;
		_height = height;
		_components = components;
		_format = format;
		_type = type;
		_target = target;

		_valid = true;
	}

	void load(const std::string &fileName, bool mipmaps = false)
	{
		destroy();

		ILuint ilhandle;
		ilGenImages(1, &ilhandle);
		ilBindImage(ilhandle);
		ilLoadImage(fileName.c_str());
		throwOnError();

		_width = ilGetInteger(IL_IMAGE_WIDTH);
		_height = ilGetInteger(IL_IMAGE_HEIGHT);
		_depth = ilGetInteger(IL_IMAGE_DEPTH);
		_components = ilGetInteger(IL_IMAGE_BYTES_PER_PIXEL);
		ILint format = ilGetInteger(IL_IMAGE_FORMAT); 
		ILint type = ilGetInteger(IL_IMAGE_TYPE);
		ILint images = ilGetInteger(IL_NUM_IMAGES);
		//ILint mips = ilGetInteger(IL_NUM_MIPMAPS);
		//ILint activelayer = ilGetInteger(IL_ACTIVE_LAYER);
		//ILint activeimage = ilGetInteger(IL_ACTIVE_IMAGE);


		_format = formatILtoGL(format);
		_type = typeILtoGL(type);
		GLenum internalformat = internalILtoGL(format, type, _components);

		unsigned char *data = ilGetData();

		// determine if image is pot or rect, if it is pot determine if it is a cube map or a 3d map
		glGenTextures(1, &_handle);
		// if its a cube map
		if(images == 5)
		{
			if(!isPOT(_width) || !isPOT(_height) || _width != _height)
				throw std::exception("Image is a cube map but either width != height, or they are not both powers of 2.");
			_target = GL_TEXTURE_CUBE_MAP;
			//glEnable(_target);
			glBindTexture(_target, _handle);
			setTextureParameters();
			for(int i=0; i<=images; i++)
			{
				ilActiveImage(i);
				if(mipmaps)
					gluBuild2DMipmaps(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, _components, _width, _height, _format, _type, ilGetData());
				else
					glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X+i, 0, internalformat, _width, _height, 0, _format, _type, ilGetData());
			}
		}
		else if(images == 0)
		{
			loadTexture2D(mipmaps, data, internalformat);
			//if(isPOT(_width) && isPOT(_height))
			//{
			//	_target = GL_TEXTURE_2D;
			//}
			//else
			//{
			//	_target = GL_TEXTURE_RECTANGLE_ARB;
			//}
			////glEnable(_target);
			//glBindTexture(_target, _handle);
			//setTextureParameters();
			//if(mipmaps)
			//	gluBuild2DMipmaps(_target, _components, _width, _height, _format, _type, data);
			//else
			//	glTexImage2D(_target, 0, internalformat, _width, _height, 0, _format, _type, data);
		}
		else
		{
			//if(!isPOT(_width) || !isPOT(_height) || _width != height)
			//	throw new Exceptions::file_error("Image is a cube map but either width != height, or they are not both powers of 2.");
		}

		ilDeleteImage(ilhandle);

		_valid = true;
	}

	void destroy()
	{
		if(_valid)
			glDeleteTextures(1, &_handle);
		_valid = false;
	}

	void bind()
	{
		glBindTexture(_target, _handle);
	}

	GLuint width() const	{ return _width; }
	GLuint height() const { return _height; }
	GLuint depth() const { return _depth; }
	GLubyte components() const { return _components; }

	GLenum format() const { return _format; }
	GLenum type() const { return _type; }

	GLuint handle() const { return _handle; }

	//const GLubyte *getDataB(GLint mipmaplevel = 0) const throw (usage_error);
	//const float *getDataF() const throw (usage_error);

	//GLuint loadGL(bool mipmaps) const;

private:
// 	template<class Archive> 
// 	void serialize(Archive & ar, const unsigned int version);

	void setTextureParameters()
	{
		if (_target == GL_TEXTURE_2D || _target == GL_TEXTURE_3D || _target == GL_TEXTURE_RECTANGLE_ARB) 
		{
			glTexParameteri(_target, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(_target, GL_TEXTURE_WRAP_T, GL_REPEAT);
			if(_target == GL_TEXTURE_3D)
				glTexParameteri(_target, GL_TEXTURE_WRAP_R, GL_REPEAT);
		}
		else if (_target == GL_TEXTURE_CUBE_MAP) 
		{
			glTexParameteri(_target, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(_target, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			glTexParameteri(_target, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		}
		glTexEnvi(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
		glTexParameteri(_target, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(_target, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
		glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);
		glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
		glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
		glPixelStorei(GL_UNPACK_SWAP_BYTES, GL_FALSE);
	}

	GLenum internalILtoGL(int ilFormat, int ilType, int components)
	{
		if(ilFormat == IL_BGR || ilFormat == IL_RGB)
			return GL_RGB8;
		if(ilFormat == IL_BGRA || ilFormat == IL_RGBA)
			return GL_RGBA8;
		if(components == 1)
			return GL_LUMINANCE8;
		return components;
	}

	GLenum formatILtoGL(int ilFormat)
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

	int formatGLtoIL(GLenum fmt)
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

	GLenum typeILtoGL(int ilType)
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

	int typeGLtoIL(GLenum type)
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

	bool isPOT(unsigned int val)
	{
		while(val != 1)
		{
			if((val & 1) != 0)
				return false;
			val >>= 1;
		}
		return true;
	}

	void throwOnError()
	{
		switch(ilGetError())
		{
		case IL_NO_ERROR:
			break;
		case ILU_OUT_OF_MEMORY:
			throw std::exception("ILU_OUT_OF_MEMORY");
		case IL_INVALID_ENUM         :
			throw std::exception("IL_INVALID_ENUM");
		case IL_FORMAT_NOT_SUPPORTED :
			throw std::exception("IL_FORMAT_NOT_SUPPORTED");
		case IL_INTERNAL_ERROR       :
			throw std::exception("IL_INTERNAL_ERROR");
		case IL_INVALID_VALUE        :
			throw std::exception("IL_INVALID_VALUE");
		case IL_ILLEGAL_OPERATION    :
			throw std::exception("IL_ILLEGAL_OPERATION");
		case IL_ILLEGAL_FILE_VALUE   :
			throw std::exception("IL_ILLEGAL_FILE_VALUE");
		case IL_INVALID_FILE_HEADER  :
			throw std::exception("IL_INVALID_FILE_HEADER");
		case IL_INVALID_PARAM        :
			throw std::exception("IL_INVALID_PARAM");
		case IL_COULD_NOT_OPEN_FILE  :
			throw std::exception("IL_COULD_NOT_OPEN_FILE");
		case IL_INVALID_EXTENSION    :
			throw std::exception("IL_INVALID_EXTENSION");
		case IL_FILE_ALREADY_EXISTS  :
			throw std::exception("IL_FILE_ALREADY_EXISTS");
		case IL_OUT_FORMAT_SAME      :
			throw std::exception("IL_OUT_FORMAT_SAME");
		case IL_STACK_OVERFLOW       :
			throw std::exception("IL_STACK_OVERFLOW");
		case IL_STACK_UNDERFLOW      :
			throw std::exception("IL_STACK_UNDERFLOW");
		case IL_INVALID_CONVERSION   :
			throw std::exception("IL_INVALID_CONVERSION");
		case IL_BAD_DIMENSIONS       :
			throw std::exception("IL_BAD_DIMENSIONS");
		case IL_FILE_READ_ERROR      : // IL_FILE_WRITE_ERROR has same value?!
			throw std::exception("IL_FILE_READ_ERROR/IL_FILE_WRITE_ERROR");
		default:
			throw new std::runtime_error("Unknown error occurred during file load!");
		};
	}
	void loadTexture2D( bool mipmaps, unsigned char * data, GLenum internalformat )
	{
		if(isPOT(_width) && isPOT(_height))
		{
			_target = GL_TEXTURE_2D;
		}
		else
		{
			_target = GL_TEXTURE_RECTANGLE_ARB;
		}
		//glEnable(_target);
		glBindTexture(_target, _handle);
		setTextureParameters();
		if(mipmaps)
			gluBuild2DMipmaps(_target, _components, _width, _height, _format, _type, data);
		else
			glTexImage2D(_target, 0, internalformat, _width, _height, 0, _format, _type, data);
	}
};

// class FUSION_API TextureManager
// {
// public:
// 	typedef std::map<std::string, TexturePtr> TextureList;
// 	typedef TextureList::iterator TextureListIterator;
// private:
// 	static boost::shared_ptr<TextureManager> _instance;
// 
// 	TextureList _textures;
// 
// public:
// 	TextureManager();
// 	~TextureManager();
// 	static TextureManager &globalManager();
// 
// 	TexturePtr load(const std::string &fileName, const std::string &textureName)
// 		throw (Exceptions::parameter_error, Exceptions::usage_error, Exceptions::memory_error);
// 	TexturePtr get(const std::string &name)
// 		throw (Exceptions::parameter_error);
// 	TextureListIterator getTextureListBegin();
// 	TextureListIterator getTextureListEnd();
// };

}