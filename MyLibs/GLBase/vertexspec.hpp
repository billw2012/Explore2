#ifndef _SCENE_VERTEXSPEC_HPP
#define _SCENE_VERTEXSPEC_HPP

#include <unordered_map>
#include <memory>
#include <algorithm>

#include "sdlgl.hpp"
#include "vramobject.hpp"

namespace glbase {;

struct VertexData { enum type {
	Nil					= 0,
	PositionData		= 1 << 0,
	NormalData			= 1 << 1,
	TexCoord0			= 1 << 2,
	TexCoord1			= 1 << 3,
	TexCoord2			= 1 << 4,
	TexCoord3			= 1 << 5
};};

struct VertexElementType { enum type {
	Nil,
	Float,
	Double
};};

struct VertexSpec : public IDObject
{
	struct VertexSpecElement;

	typedef std::vector< VertexSpecElement > VertexSpecElements;
	typedef VertexSpecElements::iterator VSIterator;
	typedef VertexSpecElements::const_iterator ConstVSIterator;
	typedef size_t size_type;
	typedef std::shared_ptr< VertexSpec > ptr;

	struct VertexSpecElement
	{
		VertexData::type label;
		size_type index;
		size_type size;
		size_type elements;
		size_type offset;
		VertexElementType::type gltype;

		VertexSpecElement() 
			: label(VertexData::Nil), 
			index(0), 
			size(0), elements(0), 
			offset(0), 
			gltype(VertexElementType::Nil) {}

		VertexSpecElement(VertexData::type type_, size_type index_, size_type size_, size_type elements_, 
			size_type offset_, VertexElementType::type gltype_)
			: label(type_), 
			index(index_), 
			size(size_), 
			elements(elements_), 
			offset(offset_), 
			gltype(gltype_) {}

		size_t hash() const 
		{
			return (size_t)label ^ index ^ size ^ elements ^ offset ^ (size_t)gltype;
		}
	};

private:
	VertexSpecElements _data;
	size_type _size;
	VertexData::type _contents;

public:

	VertexSpec() : _data(), _size(0), _contents(VertexData::Nil) {}

	VertexSpec(const VertexSpec& other)
		: _data(other._data.begin(), other._data.end()),
		_size(other._size), 
		_contents(other._contents)	{}

	VertexSpec& operator=(const VertexSpec& other)
	{
		_data.clear();
		_data.insert(_data.begin(), other._data.begin(), other._data.end());
		_size = other._size;
		_contents = other._contents;
		return *this;
	}

	void clear()
	{
		_data.clear();
		_size = 0;
		_contents = VertexData::Nil;
	}

	// get begin iterator of data offset map
	VSIterator specListBegin()
	{
		return _data.begin();
	}

	ConstVSIterator specListBegin() const
	{
		return _data.begin();
	}

	// end end iterator of data offset map
	VSIterator specListEnd()
	{
		return _data.end();
	}

	ConstVSIterator specListEnd() const
	{
		return _data.end();
	}

	// add a record describing a piece of data in the vertex
	bool append(VertexData::type label, size_type index, size_type elementsize, size_type elements, VertexElementType::type gltype)
	{
		//if(_data.find(type) != _data.end())
		//	return false;

		_data.push_back(VertexSpecElement(label, index, elements * elementsize, elements, _size, gltype));
		//_data[type].label = label;
		//_data[type].size = elementsize * elements;
		//_data[type].offset = _size;
		//_data[type].elements = elements;
		//_data[type].gltype = gltype;

		_size += elementsize * elements;
		_contents = static_cast<VertexData::type>(static_cast<unsigned int>(_contents) | static_cast<unsigned int>(label));
		return true;
	}

	// get the offset in bytes of a particular type of data in a vertex 
	size_type dataOffset(VertexData::type label) const
	{
		auto itr = std::find_if(_data.begin(), _data.end(), [&](const VertexSpecElement& elem) -> bool { return elem.label == label; });
		if(itr == _data.end())
			return 0;
		return itr->offset;
		//ConstVSIterator it = _data.find(dataType);
		//if(it == _data.end())
		//	return 0;
		//else
		//	return it->second.offset;
	}

	const VertexSpecElement* element(VertexData::type label) const
	{
		auto itr = std::find_if(_data.begin(), _data.end(), [&](const VertexSpecElement& elem) -> bool { return elem.label == label; });
		if(itr == _data.end())
			return 0;
		return &*(itr);
		//ConstVSIterator it = _data.find(dataType);
		//if(it == _data.end())
		//	return NULL;
		//else
		//	return &(it->second);
	}


	// return size in VertexType of the data for each vertex
	size_type vertexSize() const
	{
		return _size;
	}

	bool containsVertexDataType(VertexData::type label) const
	{
		auto itr = std::find_if(_data.begin(), _data.end(), [&](const VertexSpecElement& elem) -> bool { return elem.label == label; });
		return itr != _data.end();
	}

	size_type dataSize(VertexData::type label) const
	{
		auto itr = std::find_if(_data.begin(), _data.end(), [&](const VertexSpecElement& elem) -> bool { return elem.label == label; });
		if(itr == _data.end())
			return 0;
		return itr->size;
	}

	VertexData::type vertexContents() const
	{
		return _contents;
	}

	size_t hash() const
	{
		size_t val = 0;
		for(auto itr = _data.begin(); itr != _data.end(); ++itr)
			val ^= itr->hash();
		return val;
	}

	// allocate space in vertex array
	//void allocateVertices(int triSetIndex, int count);
	// get pointer to data in vertex array
	//template < class ValueType >
	//ValueType* extract(VertexData::type dataType, VertexSet& vset, size_type vertexIndex)
	//{
	//	return &vset[dataOffset(dataType) + vertexSize() * vertexIndex];
	//}
	//template < class ValueType >
	//const ValueType* const extract(VertexData::type dataType, VertexSet& vset, size_type vertexIndex) const
	//{
	//	return &vset[dataOffset(dataType) + vertexSize() * vertexIndex];
	//}

};

}
#endif // _SCENE_VERTEXSPEC_HPP