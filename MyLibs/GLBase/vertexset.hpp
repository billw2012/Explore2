#ifndef _SCENE_VERTEXSET_HPP
#define _SCENE_VERTEXSET_HPP

#include <vector>

#include "vramobject.hpp"
#include "Misc/rangelist.hpp"
#include "vertexspec.hpp"
#include "Math/vector2.hpp"
#include "Math/vector3.hpp"

namespace glbase {;

struct VertexSet : public BufferObject
{
public:
	typedef unsigned char value_type;
	typedef VertexSet this_type;
	//typedef std::vector< value_type > VertexArray;
	//typedef VertexArray::size_type size_type;
	typedef std::shared_ptr< this_type > ptr;
	typedef unsigned int handle_type;

	//typedef misc::RangeList< size_type > DirtyRangeListType;
	//typedef DirtyRangeListType::iterator DirtyRangeIterator;
	//typedef DirtyRangeListType::const_iterator DirtyRangeConstIterator;

private:
	//VertexArray _data;
	//size_type _vertCount;
	//DirtyRangeListType _dirtyRanges;
	VertexSpec::ptr _vspec;

private:
	VertexSet(const VertexSet&);
	VertexSet& operator=(const VertexSet&);

public:
	VertexSet(const VertexSpec::ptr& spec, size_t count = 0, GLenum usage = GL_STATIC_DRAW) 
		: BufferObject(GL_ARRAY_BUFFER, count * spec->vertexSize(), usage), _vspec(spec) {}
	//VertexSet(const VertexSet& other, bool deepcopy = false)
	//	: BufferObject(other), 
	//	_data(other._data),
	//	_vertCount(other._vertCount)
	//{
	//	//dirtyall();
	//	if(deepcopy)
	//	{
	//		_vspec.reset(new VertexSpec(*other._vspec));
	//	}
	//	else
	//	{
	//		_vspec = other._vspec;
	//	}
	//}
	//VertexSet(size_type count, size_type size) : _data(size * count), _vertCount(count) 
	//{
	//	//dirtyall();
	//}
	//void setSpec(VertexSpec::ptr spec) { _vspec = spec; }
	VertexSpec::ptr spec() const { return _vspec; }

	//void resize(size_t count)
	//{
	//	assert(get_handle() == 0);
	//	resize_local_buffer(count * _vspec->vertexSize());
	//}

	//template < class VertexType >
	//void push_back(const VertexType& val)
	//{
	//	assert(get_handle() == 0);
	//	//inc_change_num();
	//	for(size_t i = 0; i < sizeof(VertexType) / sizeof(value_type); ++i)
	//		_data.push_back(0);
	//	*(extract<VertexType>(_vertCount)) = val;
	//	//dirty(_vertCount, _vertCount+1);
	//	++ _vertCount;
	//}

	size_t get_count() const
	{
		return get_local_buffer_size_bytes() / _vspec->vertexSize();
	}

	//void undirty() 
	//{
	//	_dirtyRanges.clear();
	//}

	//// range is in vertices
	//void dirty(size_type start, size_type end)
	//{
	//	inc_change_num();
	//	_dirtyRanges.add(start, end);
	//}

	//void dirtyall()
	//{
	//	inc_change_num();
	//	_dirtyRanges.add(0, _vertCount);
	//}

	//DirtyRangeConstIterator beginDirtyRanges() const
	//{
	//	return _dirtyRanges.begin();
	//}

	//DirtyRangeIterator beginDirtyRanges()
	//{
	//	return _dirtyRanges.begin();
	//}

	//DirtyRangeConstIterator endDirtyRanges() const
	//{
	//	return _dirtyRanges.end();
	//}

	//DirtyRangeIterator endDirtyRanges()
	//{
	//	return _dirtyRanges.end();
	//}

	//bool isDirty() const { return !_dirtyRanges.empty(); }

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
		sync_range_bytes(start * _vspec->vertexSize(), count * _vspec->vertexSize());
	}

	void resize(size_t count)
	{
		resize_bytes(count * _vspec->vertexSize());
	}

	void reserve(size_t count)
	{
		reserve_bytes(count * _vspec->vertexSize());
	}

	template< class T >
	void push_back_t(const T& val)
	{
		push_back_t<T>(val);
	}

	//VertexArray::iterator begin() 
	//{
	//	return _data.begin();
	//}

	//VertexArray::iterator end() 
	//{
	//	return _data.end();
	//}

	//VertexArray::const_iterator begin() const
	//{
	//	return _data.begin();
	//}

	//VertexArray::const_iterator end() const
	//{
	//	return _data.end();
	//}

	template < class VertexType >
	VertexType* extract(size_t vertexIndex)
	{
		return reinterpret_cast<VertexType *>(get_local_buffer()) + vertexIndex;
	}

	template < class VertexType >
	const VertexType* const extract(size_t vertexIndex) const
	{
		return reinterpret_cast<const VertexType *>(get_local_buffer()) + vertexIndex;
	}

	// better to implement vertex iterator as it can simply be pointer addition, infact there could
	// be custom iterators for all elements in the vertex
	virtual math::Vector3f getPositionf(size_t vertexIndex) const
	{
		assert(_vspec != NULL);
		const VertexSpec::VertexSpecElement* posspec = _vspec->element(VertexData::PositionData);
		if(posspec->gltype == VertexElementType::Float)
		{
			if(posspec->elements == 2)
				return math::Vector3f(*reinterpret_cast<const math::Vector2f*>(get_local_buffer() + _vspec->vertexSize() * vertexIndex + posspec->offset));
			else
				return *reinterpret_cast<const math::Vector3f*>(get_local_buffer() + _vspec->vertexSize() * vertexIndex + posspec->offset);
		}
		else
		{
			if(posspec->elements == 2)
				return math::Vector3f(*reinterpret_cast<const math::Vector2d*>(get_local_buffer() + _vspec->vertexSize() * vertexIndex + posspec->offset));
			else
				return math::Vector3f(*reinterpret_cast<const math::Vector3d*>(get_local_buffer() + _vspec->vertexSize() * vertexIndex + posspec->offset));
		}
	}

	virtual math::Vector3d getPositiond(size_t vertexIndex) const
	{
		assert(_vspec != NULL);
		const VertexSpec::VertexSpecElement* posspec = _vspec->element(VertexData::PositionData);
		if(posspec->gltype == VertexElementType::Float)
		{
			if(posspec->elements == 2)
				return math::Vector3d(*reinterpret_cast<const math::Vector2f*>(get_local_buffer() + _vspec->vertexSize() * vertexIndex + posspec->offset));
			else
				return math::Vector3d(*reinterpret_cast<const math::Vector3f*>(get_local_buffer() + _vspec->vertexSize() * vertexIndex + posspec->offset));
			//return math::Vector3d(*reinterpret_cast<const math::Vector3f*>(&_data[_vspec->vertexSize() * vertexIndex + posspec->offset]));
		}
		else
		{
			if(posspec->elements == 2)
				return math::Vector3d(*reinterpret_cast<const math::Vector2d*>(get_local_buffer() + _vspec->vertexSize() * vertexIndex + posspec->offset));
			else
				return *reinterpret_cast<const math::Vector3d*>(get_local_buffer() + _vspec->vertexSize() * vertexIndex + posspec->offset);
			//return *reinterpret_cast<const math::Vector3d*>(&_data[_vspec->vertexSize() * vertexIndex + posspec->offset]);
		}
	}
};

}
#endif // _SCENE_VERTEXSET_HPP
