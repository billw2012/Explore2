
#include "videomemorymanager.hpp"

namespace glbase {;

enum BufferType
{
	VERTEX_ARRAY_BUFFER,
	INDEX_ARRAY_BUFFER
};

typedef unsigned int size_type;
typedef unsigned int handle_type;
//typedef std::map<handle_type, GLuint> HandleMap;
typedef std::unordered_map<handle_type, GLuint> HandleMap;

typedef unsigned int TriangleSetHandle;

struct VAOKey
{
	VAOKey(const glbase::TriangleSet::ptr& triangleSet_, const glbase::VertexSet::ptr& vertexSet_/*, const glbase::VertexSpec::ptr& vertexSpec_*/)
		: triSet(triangleSet_->get_id()), vertSet(vertexSet_->get_id())/*, vertexSpec(vertexSpec_->get_id())*/
	{}
	// DOING: convert to using VAOs, Vertex attribs, then update glsl
	// don't use the shared ptrs here, or the memory will never be freed.
	// VAO only changes when the handles change or the VertexSpec changes.
	//scene::VRamObject::vram_handle_type triSetHandle, vertSetHandle;
	size_t triSet;
	size_t vertSet;
	size_t vertexSpec;

	//bool operator==(const VAOKey& other) const 
	//{
	//	return triSetHandle == other.triSetHandle && vertSetHandle == other.vertSetHandle && vertexSpec == other.vertexSpec;
	//}

	// hasher
	size_t hash() const 
	{
		return triSet ^ vertSet /*^ vertexSpec*/;
	}

	bool operator==(const VAOKey& other) const 
	{
		return triSet == other.triSet && vertSet == other.vertSet/* && vertexSpec == other.vertexSpec*/;
	}
};

struct VAOKeyHasher
{
	size_t operator()(const VAOKey& key) const 
	{
		return key.hash();
	}
};

std::unordered_map<VAOKey, GLuint, VAOKeyHasher> _vaoHandles;

size_type _size;

GLuint _boundVAO;

std::unordered_map<size_t, size_t> _changeNums;

struct CurrentDrawData
{
	typedef std::vector<unsigned int> IndexVector;
	//IndexVector enabledClientStates;
	//IndexVector enabledTextures;
	//IndexVector vertexAttributes;
	GLvoid *indices;
	GLenum mode;
	GLenum type;
	GLsizei count;
	bool valid;
	GLuint start, end;

	CurrentDrawData() : mode(0), type(0), indices(0), count(0), valid(false), start(0), end(0) {}
};

CurrentDrawData _drawData;

// DOING:
// make VideoMemoryManager static
// finish making game run again
// convert to CMake projects
// de-templatize stuff that doesn't need to be templated.
// i.e. float/double does not need to be template param if only one can be used in a single build.
// Make it a global typedef instead.

bool VideoMemoryManager::load_buffers( const glbase::TriangleSet::ptr& triSet, const glbase::VertexSet::ptr& verts )
{
	if(triSet->get_count() == 0 || verts->get_count() == 0)
		return false;

	assert(triSet->is_created());
	assert(verts->is_created());

	

	// look for an existing VAO
	auto fItr = _vaoHandles.find(VAOKey(triSet, verts/*, vertSpec*/));
	GLuint vaoHandle;
	bool initVAO = false;
	if(fItr == _vaoHandles.end())
	{
		glGenVertexArrays(1, &vaoHandle);
		CHECK_OPENGL_ERRORS;
		fItr = _vaoHandles.insert(std::make_pair(VAOKey(triSet, verts/*, vertSpec*/), vaoHandle)).first;
		glBindVertexArray(vaoHandle);
		CHECK_OPENGL_ERRORS;
		initVAO = true;
	}
	else //if(_boundVAO != fItr->second.handle)
	{
		glBindVertexArray(fItr->second);
		CHECK_OPENGL_ERRORS;
	}

	_boundVAO = fItr->second;

	//// first create the index and vertex buffers if they don't exist yet
	//if(triSet->get_handle() == 0)
	//{
	//	GLuint handle;
	//	glGenBuffers(1, &handle);
	//	CHECK_OPENGL_ERRORS;
	//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, handle);
	//	assert(triSet->get_count() * sizeof(scene::TriangleSet::value_type) == triSet->size_bytes());
	//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, triSet->size_bytes(), &(*triSet)[0], GL_STATIC_DRAW);
	//	if(glGetError() == GL_OUT_OF_MEMORY)
	//		throw std::exception("Out of video memory!");
	//	CHECK_OPENGL_ERRORS;
	//	triSet->allocated(handle, [](scene::VRamObject::vram_handle_type vramHandle) {
	//		glDeleteBuffers(1, &vramHandle);
	//	});
	//	triSet->undirty();
	//	_changeNums[triSet->get_id()] = triSet->get_change_num();
	//}

	//if(verts->get_handle() == 0)
	//{
	//	GLuint handle;
	//	glGenBuffers(1, &handle);
	//	CHECK_OPENGL_ERRORS;
	//	glBindBuffer(GL_ARRAY_BUFFER, handle);
	//	assert(verts->count() * vertSpec->vertexSize() == verts->size_bytes());
	//	glBufferData(GL_ARRAY_BUFFER, verts->size_bytes(), &(*verts)[0], GL_STATIC_DRAW);
	//	if(glGetError() == GL_OUT_OF_MEMORY)
	//		throw std::exception("Out of video memory!");
	//	CHECK_OPENGL_ERRORS;
	//	verts->allocated(handle, [](scene::VRamObject::vram_handle_type vramHandle) {
	//		glDeleteBuffers(1, &vramHandle);
	//	});
	//	verts->undirty();
	//	_changeNums[verts->get_id()] = verts->get_change_num();
	//}

	_drawData.valid = false;

	if(initVAO)
	{
		glBindBuffer(GL_ARRAY_BUFFER, verts->get_handle());
		CHECK_OPENGL_ERRORS;
		const glbase::VertexSpec::ptr& vertSpec = verts->spec();
		for(auto itr = vertSpec->specListBegin(); itr != vertSpec->specListEnd(); ++itr)
		{
			const glbase::VertexSpec::VertexSpecElement& attrib = *itr;
			glEnableVertexAttribArray(static_cast<GLuint>(attrib.index));
			glVertexAttribPointer(static_cast<GLuint>(attrib.index), static_cast<GLint>(attrib.elements), (attrib.gltype == glbase::VertexElementType::Float)? GL_FLOAT : GL_DOUBLE, 
				FALSE, static_cast<GLsizei>(vertSpec->vertexSize()), reinterpret_cast<GLvoid *>(attrib.offset));
			CHECK_OPENGL_ERRORS;
		}
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, triSet->get_handle());
		CHECK_OPENGL_ERRORS;
	}

	//// update index and vertex data if required
	//if(triSet->isDirty() || _changeNums[triSet->get_id()] != triSet->get_change_num())
	//{
	//	if(_changeNums[triSet->get_id()] != triSet->get_change_num())
	//	{
	//		assert(triSet->get_count() * sizeof(scene::TriangleSet::value_type) == triSet->size_bytes());
	//		glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, triSet->size_bytes(), &(*triSet)[0]);
	//		CHECK_OPENGL_ERRORS;
	//	}
	//	else
	//	{
	//		for(scene::VertexSet::DirtyRangeIterator ditr = triSet->beginDirtyRanges(); ditr != triSet->endDirtyRanges(); ++ditr)
	//		{
	//			glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, ditr->first * sizeof(scene::TriangleSet::value_type), (ditr->second - ditr->first) * sizeof(scene::TriangleSet::value_type), &(*triSet)[ditr->first]);
	//			CHECK_OPENGL_ERRORS;
	//		}
	//	}
	//	triSet->undirty();
	//	_changeNums[triSet->get_id()] = triSet->get_change_num();
	//}

	//if(verts->isDirty() || _changeNums[verts->get_id()] != verts->get_change_num()) 
	//{
	//	glBindBuffer(GL_ARRAY_BUFFER, verts->get_handle());
	//	// update dirty regions
	//	if(_changeNums[verts->get_id()] != verts->get_change_num())
	//	{
	//		assert(verts->count() * vertSpec->vertexSize() == verts->size_bytes());
	//		glBufferSubData(GL_ARRAY_BUFFER, 0, verts->size_bytes(), &(*verts)[0]);
	//		CHECK_OPENGL_ERRORS;
	//	}
	//	else
	//	{
	//		for(scene::VertexSet::DirtyRangeIterator ditr = verts->beginDirtyRanges(); ditr != verts->endDirtyRanges(); ++ditr)
	//		{
	//			glBufferSubData(GL_ARRAY_BUFFER, ditr->first * vertSpec->vertexSize(), (ditr->second - ditr->first) * vertSpec->vertexSize(), &(*verts)[ditr->first * vertSpec->vertexSize()]);
	//			CHECK_OPENGL_ERRORS;
	//		}
	//	}
	//	verts->undirty();
	//	_changeNums[verts->get_id()] = verts->get_change_num();
	//}

	int indexStart = 0, indexEnd = 0;
	if(triSet->get_active_range_start() == -1)
		indexStart = 0;
	else
		indexStart = triSet->get_active_range_start();
	if(triSet->get_active_range_end() == -1)
		indexEnd = static_cast<unsigned int>(triSet->get_count());
	else
		indexEnd = triSet->get_active_range_end();

	_drawData.indices = reinterpret_cast<GLvoid*>(sizeof(glbase::TriangleSet::value_type) * (/*_drawData.end -*/ indexStart));//reinterpret_cast<GLvoid *>(getOffset(indexHandle));
	_drawData.count = static_cast<unsigned int>(indexEnd - indexStart);

	_drawData.mode = triSet->primType();

	_drawData.start = 0;
	_drawData.end = static_cast<GLuint>(verts->get_count());

	switch(sizeof(glbase::TriangleSet::value_type))
	{
	case 1:
		_drawData.type = GL_UNSIGNED_BYTE;
		assert(verts->get_count() < std::numeric_limits<unsigned __int8>::max());
		break;
	case 2:
		_drawData.type = GL_UNSIGNED_SHORT;
		assert(verts->get_count() < std::numeric_limits<unsigned __int16>::max());
		break;
	case 4:
		_drawData.type = GL_UNSIGNED_INT;
		assert(verts->get_count() < std::numeric_limits<unsigned __int32>::max());
		break;
	};

	_drawData.valid = true;

	return true;
}

void VideoMemoryManager::render_current()
{
	assert(_drawData.valid);
	//glDrawRangeElements(_drawData.mode, _drawData.start, _drawData.end, _drawData.count, _drawData.type, _drawData.indices);
	glDrawElements(_drawData.mode, _drawData.count, _drawData.type, _drawData.indices);
}

void VideoMemoryManager::unbind_buffers()
{
	glBindVertexArray(0);
	_boundVAO = 0;
}

}