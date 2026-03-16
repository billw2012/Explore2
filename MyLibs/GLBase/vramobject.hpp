#ifndef _SCENE_VRAMOBJECT_HPP
#define _SCENE_VRAMOBJECT_HPP

#include <functional>
#include <vector>
#include <limits>
//#include "videomemorymanager.hpp"
#include "sdlgl.hpp"
#include "streamingmanager.h"

namespace glbase {;

struct IDObject
{
	IDObject() : _id(++sID) {}

	size_t get_id() const { return _id; }
private:
	size_t _id;
	static size_t sID;
};

//struct VRamObject : public IDObject
//{
//	VRamObject();
//
//	void generate();
//	void destroy();
//	GLuint get_handle() const { return _handle; }
//
//	bool is_handle_valid() const { return _handle != std::numeric_limits<GLuint>::max(); }
//
//	virtual void bind() = 0;
//	//virtual void unbind() = 0;
//
//protected:
//	virtual GLuint generate_internal() = 0;
//	virtual void destroy_internal(GLuint handle) = 0;
//
//private:
//	GLuint _handle;
//
//};

 
struct BufferObject : public IDObject
{
	BufferObject(GLenum target, size_t bytesSize, GLenum usage = GL_STATIC_DRAW)
		: _target(target),
		_usage(usage),
		_buffer(bytesSize),
		_created(false),
		_handle(std::numeric_limits<GLuint>::max()),
		_changeNum(0)
	{}

	~BufferObject() { destroy(); }

	GLuint get_handle() const { return _handle; }
	bool is_handle_valid() const { return _handle != std::numeric_limits<GLuint>::max(); }
	void bind();

	const unsigned char* const get_local_buffer() const { return &_buffer[0]; }
	unsigned char* get_local_buffer() { return &_buffer[0]; }
	size_t get_local_buffer_size_bytes() const { return _buffer.size(); }
	size_t get_change_num() const { return _changeNum; }

	bool is_created() const { return _created; }

protected:
	void generate();
	void destroy();

	void reserve_bytes(size_t bytes)
	{
		// can't resize after create!
		assert(!_created);
		_buffer.reserve(bytes);
	}

	void resize_bytes(size_t bytes)
	{
		// can't resize after create!
		assert(!_created);
		_buffer.reserve(bytes);
	}

	template< class T >
	void push_back_t(const T& val)
	{
		// can't resize after create!
		assert(!_created);
		_buffer.resize(_buffer.size() + sizeof(T));
		::memcpy_s(static_cast<void*>(&_buffer[_buffer.size() - sizeof(T)]), sizeof(T), static_cast<const void*>(&val), sizeof(T));
	}

	void sync_range_bytes(size_t start = 0, size_t count = 0);

private:
	std::vector<unsigned char> _buffer;
	GLenum _target;
	GLenum _usage;
	bool _created;
	size_t _changeNum;
	GLuint _handle;
};

}

#endif // _SCENE_VRAMOBJECT_HPP
