#ifndef _SCENE_TRIANGLESET_HPP
#define _SCENE_TRIANGLESET_HPP

#include <vector>
#include "vramobject.hpp"
#include "Math/aabb.hpp"
#include "Math/boundingsphere.hpp"
#include "vertexset.hpp"


namespace glbase {;

struct TrianglePrimitiveType { enum type {
	POINT			= GL_POINTS,
	TRIANGLES		= GL_TRIANGLES,
	TRIANGLE_STRIP	= GL_TRIANGLE_STRIP,
	TRIANGLE_FAN	= GL_TRIANGLE_FAN,
	QUADS			= GL_QUADS,
	LINES			= GL_LINES,
	LINE_STRIP		= GL_LINE_STRIP,
	LINE_LOOP		= GL_LINE_LOOP
};};

struct TriangleSet : public BufferObject
{
	typedef unsigned short value_type;
	//typedef std::vector< value_type > IndexArray;
	//typedef IndexArray::iterator IndexIterator;
	//typedef IndexArray::const_iterator ConstIndexIterator;
	//typedef IndexArray::size_type size_type;
	typedef std::shared_ptr< TriangleSet > ptr;

	//typedef misc::RangeList< size_type > DirtyRangeListType;
	//typedef DirtyRangeListType::iterator DirtyRangeIterator;
	//typedef DirtyRangeListType::const_iterator DirtyRangeConstIterator;

private:
	//IndexArray _data;
	TrianglePrimitiveType::type _type;
	//DirtyRangeListType _dirtyRanges;
	//bool _resized;
	int _activeRangeStart, _activeRangeEnd;

private:
	TriangleSet(const TriangleSet&);
	TriangleSet& operator=(const TriangleSet&);

public:
	TriangleSet(TrianglePrimitiveType::type type, size_t count = 0, GLenum usage = GL_STATIC_DRAW) 
		: BufferObject(GL_ELEMENT_ARRAY_BUFFER, count * sizeof(value_type), usage), 
		_type(type), 
		_activeRangeStart(-1), 
		_activeRangeEnd(-1) {}


	TrianglePrimitiveType::type primType() const
	{
		return _type;
	}

	size_t get_count() const
	{
		return get_local_buffer_size_bytes() / sizeof(value_type);
	}

	value_type& operator()(size_t idx)
	{
		return *(reinterpret_cast<value_type *>(get_local_buffer()) + idx);
	}

	const value_type& operator[](size_t idx) const
	{
		return *(reinterpret_cast<const value_type *>(get_local_buffer()) + idx);
	}

	void sync_all()
	{
		sync_range_bytes();
	}

	void sync_range(size_t start, size_t count)
	{
		if(count == 0)
			count = get_count() - start;
		sync_range_bytes(start * sizeof(value_type), count * sizeof(value_type));
	}

	void resize(size_t count)
	{
		resize_bytes(count * sizeof(value_type));
	}

	void reserve(size_t count)
	{
		reserve_bytes(count * sizeof(value_type));
	}

	void push_back(value_type idx)
	{
		push_back_t<value_type>(idx);
	}

	//void push_back(value_type val)
	//{
	//	assert(get_handle() == 0);
	//	_data.push_back(val);
	//}

	void set_active_range(int start, int end)
	{
		_activeRangeStart = start;
		_activeRangeEnd = end;
	}

	void reset_active_range()
	{
		_activeRangeStart = _activeRangeEnd = -1;
	}

	int get_active_range_start() const 
	{
		return _activeRangeStart;
	}

	int get_active_range_end() const 
	{
		return _activeRangeEnd;
	}
};

}

#endif // _SCENE_TRIANGLESET_HPP
