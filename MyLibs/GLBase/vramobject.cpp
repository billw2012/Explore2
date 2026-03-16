#include "vramobject.hpp"
#include "sdlgl.hpp"


namespace glbase {;

//const size_t VRamObject::INVALID_CHANGENUM = 0;
size_t IDObject::sID = 0;

//struct BufferStreamingCommand : public StreamingCommand
//{
//	BufferStreamingCommand(GLuint handle, GLenum target, const void* ptr, size_t count)
//		: _handle(handle), 
//		_target(target),
//		_ptr(static_cast<const unsigned char*>(ptr)),
//		_countRemaining(count) {}
//
//	virtual size_t process(size_t maxToSend)
//	{
//		// add a little extra as we don't want to come back next frame for a tiny extra
//		// amount of data
//		size_t amountToSend = std::min<size_t>(maxToSend * 1.2, _countRemaining);
//		glBindBuffer(_target, _handle);
//		glBufferSubData(_target, 0, amountToSend, _ptr);
//		_ptr += amountToSend;
//		_countRemaining -= amountToSend;
//		return maxToSend - std::min<size_t>(maxToSend, amountToSend);
//	}
//
//	virtual bool is_complete() const
//	{
//		return _countRemaining == 0;
//	}
//
//	virtual void cancel() const
//	{
//
//	}
//
//private:
//	GLuint _handle;
//	GLenum _target;
//	const unsigned char* _ptr;
//	size_t _countRemaining;
//};

// VRamObject --------------------------------------------------------------------

//VRamObject::VRamObject() : _handle(std::numeric_limits<GLuint>::max())
//{
//
//}
//
//VRamObject::~VRamObject()
//{
//	destroy();
//}
//
//void VRamObject::generate()
//{
//	if(!is_handle_valid())
//		_handle = generate_internal();
//}
//
//void VRamObject::destroy()
//{
//	destroy_internal(_handle);
//}


// BufferObject --------------------------------------------------------------------
//size_t BufferObject::get_size_bytes() const
//{
//	return _buffer.size();
//}

void BufferObject::bind()
{
	assert(is_handle_valid());
	::glBindBuffer(_target, get_handle());
}

void BufferObject::generate()
{
	if(!is_handle_valid())
		::glGenBuffers(1, &_handle);
}

void BufferObject::destroy()
{
	::glDeleteBuffers(1, &_handle);
}

void BufferObject::sync_range_bytes( size_t start /*= 0*/, size_t count /*= 0*/ )
{
	generate();
	::glBindBuffer(_target, get_handle());

	if(count == 0)
		count = _buffer.size() - start;
	if(!_created && start == 0 && count == _buffer.size())
	{
		_created = true;
		::glBufferData(_target, _buffer.size(), &_buffer[0], _usage);
	}
	else if(!_created)
	{
		_created = true;
		::glBufferData(_target, _buffer.size(), NULL, _usage);
	}
	else
	{
		::glBufferSubData(_target, start, count, &_buffer[start]);
	}
	++_changeNum;
}

}
