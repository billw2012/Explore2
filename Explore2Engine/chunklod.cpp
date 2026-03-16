
#include "chunklod.h"
//#define BOOST_BIND_ENABLE_STDCALL

#include "GLBase/videomemorymanager.hpp"

/*

Transition from planet to chunklod water:
water colour starts as planet texture, blends to chunk water colour.
verts blend from planet to chunk positions.

*/


namespace explore2 { ;

using namespace math;
using namespace scene;
using namespace scene::transform;

//struct AbortedChunksManager
//{

std::unordered_set<ChunkLOD::ptr> gChunksToDelete;
//};

void ChunkLOD::delete_chunk(ChunkLOD::ptr chunk)
{
	//if(chunk->is_building())
	//{
	//	chunk->abort_build();
	//	//gChunksToDelete.insert(chunk);
	//}
	//}	//assert(chunk->get_state() <= State::Prepared);
	chunk->set_state(State::Empty);
	assert(chunk->_state.get_state() == State::Empty);
	assert(!chunk->_mesh || !chunk->_shared->geometry->contains(chunk->_mesh));
	//assert(chunk->_mesh == NULL);
}

void ChunkLOD::delete_aborted_chunks()
{
	std::vector< /*std::set<*/ChunkLOD::ptr/*>::iterator*/ > toDelete;
	for(auto cItr = gChunksToDelete.begin(); cItr != gChunksToDelete.end(); ++cItr)
	{
		ChunkLOD::ptr chunk = *cItr;
		chunk->abort_build();
		chunk->check_built();
		if(!chunk->is_building())
		{
			toDelete.push_back(chunk);
			//gChunksToDelete.erase(cItr);
		}
	}

	for(size_t idx = 0; idx < toDelete.size(); ++idx)
	{
		gChunksToDelete.erase(toDelete[idx]);
	}
}

// create as root
ChunkLOD::ChunkLOD(SharedData* shared,
				   scene::transform::Group::ptr rootTransform,
				   const std::vector<math::Vector3f>& sourcePts,
				   glbase::Texture::ptr parentNormalTexture,
				   glbase::Texture::ptr parentColourTexture,
				   const math::Rectanglef& parentUVWindow)
	: _shared(shared),
	_parent(NULL), 
	_level(0),
	_rootTransform(rootTransform),
	_state(State::Empty),
	//_buildThreadData(new ChunkLODThreadData(shared->heightData, shared->textureData)),
	_buildingState(BuildingState::NotBuilding),
	//_childShowError(10000.0f),
	//_childCreateError(10000.0f),
	_sourcePts(sourcePts),
	_parentNormalTexture(parentNormalTexture),
	_parentColourTexture(parentColourTexture),
	_parentNormalUVWindow(parentUVWindow),
	_parentTextureUVWindow(parentUVWindow),
	_textureUVWindow(0.0, (shared->gridSize - 1) * 2, (shared->gridSize - 1) * 2, 0.0),
	_currentBlendWeight(0),
	_currentClosestDistance(0),
	_batched(false),
	_landW(0),
	_waterW(0),
	_chunkLandCache(new BatchLandCache(16, shared->textureRes, shared->gridSize * 2, 
		shared->gridSize * shared->gridSize + shared->gridSize * 4, _batchLandVertexSpec, rootTransform, shared->geometry, shared->texturingFBO)),
		_chunkWaterCache(new BatchWaterCache(16, shared->gridSize * shared->gridSize + shared->gridSize * 4, _batchWaterVertexSpec, rootTransform, shared->geometry)),
	_oneWeightFrames(0)
{
	init_state();

	for(size_t idx = 0; idx < _sourcePts.size(); ++idx)
	{
		_localBounds.expand(_sourcePts[idx]);
	}

	_error = (_localBounds.max() - _localBounds.min()).length() * 1;

	_childError = _error / 2;
}

// to optimize the graphics pipeline:
// have shaders work out the texture units that textures are to bind to
// write a texture binding wrapper that checks to see if 
// a texture is already bound to a unit.
// sort by renderstage -> alpha -> shader -> texture -> autobind parameters 
// -> custom parameters.

// before above:
// add all visible chunks to a queue system, sort by last modified time. Those that 
// have been unmodified for more than x time, group into batches.
// also use set of 5/10/20 shaders with fixed interpolations and switch between them, 
// so that groups can be grouped by them?

// also:
// packing textures = easy, all textures are the same res., we know where there is space
// and we know how many textures are packed into each pack texture. can decide when
// to repack based on the usage of each packed texture.
// to make this generic there should be an interface class:
/*
struct BatchedVertex
{
	Vector3f pos;
	Vector2f normalUV;
}
struct BatchableChunk
{
	virtual void bake_verts() = 0;
	virtual void recalc_uvs(Rectanglef newUVArea) = 0;
};
*/
// how does baking work with texture blending? bake textures as well?
// should we only bake when a chunk has a 1.0 weight? (removing the need for 
// texture baking). or just make the texture baking smart enough to not do
// blending if weight is 0 or 1.
// determine lod blend by using point -> bounding box distance algorithm 
// (whatever that is). shift all blending to shaders? use cg instead of cgfx and write
// my own state manager? 

template < class Ty_ >
Ty_ interpolate(const Ty_& a, const Ty_& b, double t)
{
	return a + (b - a) * t;
}

// create from parent
ChunkLOD::ChunkLOD(const math::Rectanglei& parentArea, 
				   const math::Rectanglef& parentUVWindow, 
				   ChunkLOD* parent)
	: _shared(parent->_shared),
	_parent(parent), 
	_rootTransform(parent->_rootTransform),
	_level(parent->_level+1),
	_error(parent->_childError),
	_state(State::Empty),
	//_buildThreadData(new ChunkLODThreadData(parent->_shared->heightData, parent->_shared->textureData)),
	_parentArea(parentArea),
	_buildingState(BuildingState::NotBuilding),
// 	_childShowError(10000.0f),
// 	_childCreateError(10000.0f),
	_parentColourTexture(parent->_colourTexture),
	_parentNormalTexture(parent->_normalTexture),
	//_parentTextureUVWindow(parentUVWindow),
	//_parentNormalUVWindow(parentUVWindow),
	_currentBlendWeight(0),
	_currentClosestDistance(0),
	_landW(0),
	_waterW(0),
	_chunkLandCache(parent->_chunkLandCache),
	_chunkWaterCache(parent->_chunkWaterCache),
	_oneWeightFrames(0),
	_batched(false)
{
	init_state();

	_childError = _error / 2;

	assert(_parent->is_built());

	ChunkVertex* parentTopLeft = _parent->_mesh->get_verts()->extract
		<ChunkVertex>(get_vert_idx(_parentArea.left, _parentArea.top));
	_localBounds.expand(parentTopLeft->pos);
	_parentNormalUVWindow.left = parentTopLeft->normalUV.x;
	_parentNormalUVWindow.top = parentTopLeft->normalUV.y;
	_parentTextureUVWindow.left = parentTopLeft->textureUV.x;
	_parentTextureUVWindow.top = parentTopLeft->textureUV.y;
	_localBounds.expand(_parent->_mesh->get_verts()->extract
		<ChunkVertex>(get_vert_idx(_parentArea.right, _parentArea.top))->pos);
	ChunkVertex* parentBottomRight = _parent->_mesh->get_verts()->extract
		<ChunkVertex>(get_vert_idx(_parentArea.right, _parentArea.bottom));
	_parentNormalUVWindow.right = parentBottomRight->normalUV.x;
	_parentNormalUVWindow.bottom = parentBottomRight->normalUV.y;
	_parentTextureUVWindow.right = parentBottomRight->textureUV.x;
	_parentTextureUVWindow.bottom = parentBottomRight->textureUV.y;
	_localBounds.expand(parentBottomRight->pos);
	_localBounds.expand(_parent->_mesh->get_verts()->extract
		<ChunkVertex>(get_vert_idx(_parentArea.left, _parentArea.bottom))->pos);

	_textureUVWindow = math::Rectangled(
		interpolate(_parent->_textureUVWindow.left, _parent->_textureUVWindow.right, parentUVWindow.left),
		interpolate(_parent->_textureUVWindow.top, _parent->_textureUVWindow.bottom, parentUVWindow.bottom),
		interpolate(_parent->_textureUVWindow.left, _parent->_textureUVWindow.right, parentUVWindow.right),
		interpolate(_parent->_textureUVWindow.top, _parent->_textureUVWindow.bottom, parentUVWindow.top));

	create_geometry();
}

ChunkLOD::~ChunkLOD()
{
	//abort_build();
	//wait_for_build();
	//State::type initialDeleteState = _state.get_state();
	bool res = set_state(State::Empty);
	//assert(res);
	//assert(!_children[0]);
	//assert(!_mesh);
	// _mesh may not be null IF all the following are true:
	// this chunk is a child chunk
	// its parent has switched to PrepareChildren state
	// the chunk has 1 or more siblings that are not built
	//if(_mesh != NULL)
	//{
	//	//assert(_parent != NULL);
	//	//assert(_parent->get_state() == State::PrepareChildren);
	//	//assert(!_parent->are_children_built());
	//	delete_geometry_data();
	//}
}

void ChunkLOD::init_state()
{
	_state.add_transition_up_func(State::Empty, std::bind(&ChunkLOD::create_geometry, this));
	_state.add_transition_up_func(State::Prepared, std::bind(&ChunkLOD::show, this));
	_state.add_transition_up_func(State::Shown, std::bind(&ChunkLOD::create_children, this));
	_state.add_transition_up_func(State::PrepareChildren, std::bind(&ChunkLOD::show_children_and_hide, this));

	_state.add_transition_down_func(State::ChildrenShown, std::bind(&ChunkLOD::hide_children_and_show, this));
	_state.add_transition_down_func(State::PrepareChildren, std::bind(&ChunkLOD::delete_children, this));
	_state.add_transition_down_func(State::Shown, std::bind(&ChunkLOD::hide, this));
	_state.add_transition_down_func(State::Prepared, std::bind(&ChunkLOD::delete_geometry, this));
}

bool ChunkLOD::set_state(State::type state)
{
	bool res =  _state.set_state(state);
#if defined(_DEBUG)
	if(res) 
	{
		_DEBUG_stateChanges.push_front(state);
		if(_DEBUG_stateChanges.size() > 50)
			_DEBUG_stateChanges.pop_back();
	}
#endif
	return res;
}

float ChunkLOD::calculate_distance_2( const Vector3f& localCameraPos ) const 
{
	float shortestD = (_localBounds.closest_point(localCameraPos) - localCameraPos).lengthSquared();
	return shortestD;
}

template < class VertTy_ >
void update_skirt_UVs_from_edge_UVs(glbase::VertexSet::ptr verts, int gridSize)
{
	int vidx = gridSize * gridSize;
	for(int x = 0; x < gridSize; ++x, ++vidx)
	{
		size_t idx = get_vert_idx(x, 0, gridSize);
		VertTy_* edgeVert = verts->extract<VertTy_>(idx);
		VertTy_* newVert = verts->extract<VertTy_>(vidx);
		transfer_uvs(edgeVert, newVert);
	}
	for(int y = 0; y < gridSize; ++y, ++vidx)
	{
		size_t idx = get_vert_idx(gridSize-1, y, gridSize);
		VertTy_* edgeVert = verts->extract<VertTy_>(idx);
		VertTy_* newVert = verts->extract<VertTy_>(vidx);
		transfer_uvs(edgeVert, newVert);
	}
	for(int x = gridSize-1; x >= 0; --x, ++vidx)
	{
		size_t idx = get_vert_idx(x, gridSize-1, gridSize);
		VertTy_* edgeVert = verts->extract<VertTy_>(idx);
		VertTy_* newVert = verts->extract<VertTy_>(vidx);
		transfer_uvs(edgeVert, newVert);
	}
	for(int y = gridSize-1; y >= 0; --y, ++vidx)
	{
		size_t idx = get_vert_idx(0, y, gridSize);
		VertTy_* edgeVert = verts->extract<VertTy_>(idx);
		VertTy_* newVert = verts->extract<VertTy_>(vidx);
		transfer_uvs(edgeVert, newVert);
	}
}


void ChunkLOD::update_parent_textures(glbase::Texture::ptr parentNormalTexture,
							glbase::Texture::ptr parentColourTexture,
							const math::Rectanglef& newParentUVWindow)
{
	_parentColourTexture = parentColourTexture;
	_parentNormalTexture = parentNormalTexture;
	if(_mesh)
	{
		_mesh->get_material()->set_parameter("ParentColourTexture", _parentColourTexture);
		_mesh->get_material()->set_parameter("ParentNormalTexture", _parentNormalTexture);
	}
	if(is_root() && _waterMesh)
	{
		_waterMesh->get_material()->set_parameter("ParentColourTexture", _parentColourTexture);
		_waterMesh->get_material()->set_parameter("ParentNormalTexture", _parentNormalTexture);
	}
	_parentNormalUVWindow = newParentUVWindow;
	if(is_built())
	{
		update_UVs();
		update_skirt_UVs_from_edge_UVs < ChunkVertex > (_mesh->get_verts(), (int)_shared->gridSize);
		_mesh->get_verts()->sync_all();
		if(is_root() && _waterMesh)
		{
			update_skirt_UVs_from_edge_UVs < ChunkWaterL0Vertex > (_waterMesh->get_verts(), (int)_shared->gridSize);
			_waterMesh->get_verts()->sync_all();
		}
	}
}

// TODO:
// skirts- 
// each chunk should have its own skirt: 
// tri strip along edge of skirt, with top vert as the edge vert and
// bottom vert as a static vert some distance below.
// the static verts are to be added at the end of the blended verts so that 
// that a single vert list can be used.
// all planet polygons should have a skirt around them. only activate them when some chunks are visible.
// only activate those that border active chunks- how to work that at the edges of faces? just map them,
// and do it procedurally. do it heuristically by activating all those in range, maybe all chunks whose 
// top levels are built (should be a ring that surrounds those that are already shown?)


ChunkLOD::Errors ChunkLOD::calculate_errors(const Vector3f& localCameraPos, Errors& errors) const
{
	// calculate the shortest distance from the camera to the bounding box
	float dist_2 = calculate_distance_2(localCameraPos);
	float shortestD_2_recip;

	if(dist_2 != 0.0f)
		shortestD_2_recip = 1.0f / dist_2;
	else
		shortestD_2_recip = std::numeric_limits<float>::max();

	// calculate error value
	//float selfCreateError, selfShowError;
	if(_parent == NULL)
	{
		errors.create = std::pow(_error * 2, 2) * shortestD_2_recip;
		errors.show = std::pow(_error, 2) * shortestD_2_recip;
	}

	float offs = 0.0001f;
	Errors childErrors;
	childErrors.show = std::min(std::pow(_childError, 2) * shortestD_2_recip + offs, errors.show);
	childErrors.create = std::max((errors.show + childErrors.show) * 0.5f, childErrors.show);
	return childErrors;
}

void ChunkLOD::update(const Vector3f& localCameraPos, const Vector3f& predictedCameraPos, Errors errors /*= Errors()*/)
{
	// if we are active based on localCameraPos but not based on predictedCameraPos then DO NOT create the chunk
	
	//// calculate the shortest distance from the camera to the bounding box
	//float dist_2 = calculate_distance_2(localCameraPos);
	//float shortestD_2_recip;

	//if(dist_2 != 0.0f)
	//	shortestD_2_recip = 1.0f / dist_2;
	//else
	//	shortestD_2_recip = std::numeric_limits<float>::max();

	//// calculate error value
	//float selfCreateError, selfShowError;
	//if(_parent != NULL)
	//{
	//	selfCreateError = errors.create; //_parent->get_child_create_error();
	//	selfShowError = errors.show; //_parent->get_child_show_error();
	//}	
	//else
	//{
	//	selfCreateError = std::pow(_error * 2, 2) * shortestD_2_recip;
	//	selfShowError = std::pow(_error, 2) * shortestD_2_recip;
	//}

	//float offs = 0.0001f;
	//Errors childErrors;
	//childErrors.show = std::min(std::pow(_childError, 2) * shortestD_2_recip + offs, selfShowError);
	//childErrors.create = std::max((selfShowError + childErrors.show) * 0.5f, childErrors.show);

	Errors predictedErrors = errors;
	Errors childErrors = calculate_errors(localCameraPos, errors);
	Errors predictedChildErrors = calculate_errors(predictedCameraPos, predictedErrors);

	float unclampedWeight = (1.0f - childErrors.show) / (errors.show - childErrors.show);
	assert(unclampedWeight != std::numeric_limits<float>::quiet_NaN() &&
		unclampedWeight != std::numeric_limits<float>::signaling_NaN());

	_currentBlendWeight = 1 - math::clamp(unclampedWeight - 0.5f, 0.0f, 0.5f) * 2.0f;

	if(get_state() >= State::ChildrenShown)
		update_children(localCameraPos, predictedCameraPos, childErrors);

	// if we should be building children
	if(childErrors.show > 1 && predictedChildErrors.show > 1)
	{
		set_state(State::ChildrenShown);
	}
	else if(childErrors.create > 1 && predictedChildErrors.create > 1)
	{
		set_state(State::PrepareChildren);
	}
	else if(!is_root()) 
	{
		// drop down one more state if we are child:
		// if parent says children are shown then we drop to shown, otherwise 
		// drop to prepared
		if(_parent->_state.get_state() == State::ChildrenShown)
		{
			set_state(State::Shown);
		}
		else
		{
			set_state(State::Prepared);
		}
	}
	else
	{
		if(errors.show > 1 && predictedErrors.show > 1)
		{
			set_state(State::Shown);
		}
		else if(errors.create > 1 && predictedErrors.create > 1)
		{
			set_state(State::Prepared);
		}
		else
		{
			set_state(State::Empty);
		}
	}

	if(_mesh/* && !_batched*/)
	{
		float sqrtBlendWeight = std::sqrt(_currentBlendWeight);
		_landW = sqrtBlendWeight;
		if(is_root() && _waterMesh)
		{
			_landW = math::clamp<float>(sqrtBlendWeight * 2, 0, 1);
			_waterW = math::clamp<float>(sqrtBlendWeight * 2 - 1, 0, 1);
			_waterMesh->get_material()->set_parameter("BlendFactor", _waterW);
		}
		_mesh->get_material()->set_parameter("BlendFactor", _landW);
	}

	if(_landW > 0.99f)
		++_oneWeightFrames;
	else
		_oneWeightFrames = 0;

	// update batching
	if(_state.get_state() == State::Shown || _state.get_state() == State::PrepareChildren)
	{
#if 0
		if(_oneWeightFrames >= 20 && !_batched)
		{
			_shared->geometry->erase(_mesh);
			_chunkLandCache->add(this);
			if(_waterMesh)
			{
				_shared->geometry->erase(_waterMesh);
				_chunkWaterCache->add(this);
			}
			_batched = true;
		}
		else if(_oneWeightFrames < 20 && _batched)
		{
			_shared->geometry->insert(_mesh);
			_chunkLandCache->remove(this);
			if(_waterMesh)
			{
				_shared->geometry->insert(_waterMesh);
				_chunkWaterCache->remove(this);
			}
			_batched = false;
		}
#endif	
	}
	//else if(_batched)
	//{
	//	_chunkLandCache->remove(this);
	//	_batched = false;
	//}

	if(_level == 0)
	{
		_chunkLandCache->update();
		_chunkWaterCache->update();
	}
}



void ChunkLOD::hide_all()
{
	if(are_children_created())
	{
		for(size_t idx = 0; idx < 4; ++idx)
		{
			_children[idx]->hide_all();
		}
	}
	hide();
}

void draw_texture_vertex(float u, float v, float weight, int x, int y)
{
	glMultiTexCoord3f(GL_TEXTURE0, u, v, weight);
	glVertex2i(x, y);
}

void set_skirt_pos(ChunkVertex* vert, const math::Vector3f& newPos)
{
	vert->pos = newPos;
	vert->posParent = newPos;
}

void set_skirt_pos(ChunkWaterVertex* vert, const math::Vector3f& newPos)
{
	vert->pos = newPos;
}

void set_skirt_pos(ChunkWaterL0Vertex* vert, const math::Vector3f& newPos)
{
	vert->pos = newPos;
	vert->posParent = newPos;
}

size_t get_vert_idx(size_t x, size_t y, size_t gridSize)
{
	return y * gridSize + x;
}

template < class VertType_ >
void create_skirt_verts( int vidx, const glbase::VertexSet::ptr& verts, float skirtHeightScale, 
					   const math::Matrix4d& trans, const math::Matrix4d& inverseTrans, 
					   const glbase::TriangleSet::ptr& skirtTris, int gridSize ) 
{
	for(int x = 0; x < gridSize; ++x, ++vidx)
	{
		size_t idx = get_vert_idx(x, 0, gridSize);
		const VertType_* edgeVert = verts->extract<VertType_>(idx);
		math::Vector3d globalPos(matrix_transform<double>(trans, math::Vector3d(edgeVert->pos)));
		globalPos -= globalPos.normal() * skirtHeightScale;
		globalPos = matrix_transform<double>(inverseTrans, globalPos);
		VertType_* newVert = verts->extract<VertType_>(vidx);
		*newVert = *edgeVert;
		set_skirt_pos(newVert, math::Vector3f(globalPos));
		if(skirtTris)
		{
			skirtTris->push_back(vidx);
			skirtTris->push_back((glbase::TriangleSet::value_type)idx);
		}
	}
	for(int y = 0; y < gridSize; ++y, ++vidx)
	{
		size_t idx = get_vert_idx(gridSize-1, y, gridSize);
		const VertType_* edgeVert = verts->extract<VertType_>(idx);
		math::Vector3d globalPos = matrix_transform<double>(trans, math::Vector3d(edgeVert->pos));
		globalPos -= globalPos.normal() * skirtHeightScale;
		globalPos = matrix_transform<double>(inverseTrans, globalPos);
		VertType_* newVert = verts->extract<VertType_>(vidx);
		*newVert = *edgeVert;
		set_skirt_pos(newVert, math::Vector3f(globalPos));
		if(skirtTris)
		{
			skirtTris->push_back(vidx);
			skirtTris->push_back((glbase::TriangleSet::value_type)idx);
		}
	}
	for(int x = gridSize-1; x >= 0; --x, ++vidx)
	{
		size_t idx = get_vert_idx(x, gridSize-1, gridSize);
		const VertType_* edgeVert = verts->extract<VertType_>(idx);
		math::Vector3d globalPos = matrix_transform<double>(trans, math::Vector3d(edgeVert->pos));
		globalPos -= globalPos.normal() * skirtHeightScale;
		globalPos = matrix_transform<double>(inverseTrans, globalPos);
		VertType_* newVert = verts->extract<VertType_>(vidx);
		*newVert = *edgeVert;
		set_skirt_pos(newVert, math::Vector3f(globalPos));
		if(skirtTris)
		{
			skirtTris->push_back(vidx);
			skirtTris->push_back((glbase::TriangleSet::value_type)idx);
		}
	}
	size_t lastSkirtIdx;
	for(int y = gridSize-1; y >= 0; --y, ++vidx)
	{
		lastSkirtIdx = get_vert_idx(0, y, gridSize);
		const VertType_* edgeVert = verts->extract<VertType_>(lastSkirtIdx);
		math::Vector3d globalPos = matrix_transform<double>(trans, math::Vector3d(edgeVert->pos));
		globalPos -= globalPos.normal() * skirtHeightScale;
		globalPos = matrix_transform<double>(inverseTrans, globalPos);
		VertType_* newVert = verts->extract<VertType_>(vidx);
		*newVert = *edgeVert;
		set_skirt_pos(newVert, math::Vector3f(globalPos));
		if(skirtTris)
		{
			skirtTris->push_back(vidx);
			skirtTris->push_back((glbase::TriangleSet::value_type)lastSkirtIdx);
		}
	}
	if(skirtTris)
		skirtTris->push_back((glbase::TriangleSet::value_type)lastSkirtIdx);
}

void ChunkLOD::create_water_verts_not_root()
{
	for(int y = 0, vidx = 0; y < _shared->gridSize; ++y)
	{
		for(int x = 0; x < _shared->gridSize; ++x, ++vidx)
		{
			*_waterMesh->get_verts()->extract<ChunkWaterVertex>(vidx) = 
				ChunkWaterVertex(math::Vector3f(_buildThreadData->get_child_water_pos(x * 2, y * 2)));
		}
	}
}

void ChunkLOD::create_water_verts_root()
{
	for(int y = 0, vidx = 0; y < _shared->gridSize; ++y)
	{
		for(int x = 0; x < _shared->gridSize; ++x, ++vidx)
		{
			ChunkWaterL0Vertex* vert = _waterMesh->get_verts()->extract<ChunkWaterL0Vertex>(vidx);
			vert->pos = _buildThreadData->get_child_water_pos(x * 2, y * 2);
			vert->posParent = _buildThreadData->get_child_parent_pos(x * 2, y * 2);
		}
	}
}

void ChunkLOD::create_verts()
{
	// offset normal map uvs by half a pixel to make sure the sampling works correctly
	// (base map is twice the resolution of the grid..)
	int vidx = 0;
	for(int y = 0; y < _shared->gridSize; ++y)
	{
		for(int x = 0; x < _shared->gridSize; ++x, ++vidx)
		{
			ChunkVertex* vert = _mesh->get_verts()->extract<ChunkVertex>(vidx);
			vert->pos = _buildThreadData->get_child_pos(x * 2, y * 2);
			vert->posParent = _buildThreadData->get_child_parent_pos(x * 2, y * 2);
		}
	}
}

void ChunkLOD::update_UVs()
{
	bool hasWaterAndIsRoot = _buildThreadData->has_water() && is_root();
	// offset normal map uvs by half a pixel to make sure the sampling works correctly
	// (base map is twice the resolution of the grid..)
	float uvTexelSize = 1 / static_cast<float>((_shared->gridSize - 1) * 2);
	float uvOffset = 0.5f * uvTexelSize;
	float normalV = uvOffset, 
		normalDUV = (1 - uvTexelSize) / static_cast<float>(_shared->gridSize - 1), 
		parentNormalV = _parentNormalUVWindow.top,
		parentNormalDV = (_parentNormalUVWindow.bottom - _parentNormalUVWindow.top) / static_cast<float>(_shared->gridSize-1);
	float textureV = (0.5f / static_cast<float>(_shared->textureRes)), 
		textureDUV = (1 / static_cast<float>(_shared->gridSize - 1)),
		parentTextureV = _parentTextureUVWindow.top,
		parentTextureDV = (_parentTextureUVWindow.bottom - _parentTextureUVWindow.top) / static_cast<float>(_shared->gridSize-1);
	int vidx = 0;
	for(int y = 0; y < _shared->gridSize; 
		++y, textureV += textureDUV, parentTextureV += parentTextureDV,
		normalV += normalDUV, parentNormalV += parentNormalDV)
	{
		float normalU = uvOffset,
			parentNormalU = _parentNormalUVWindow.left,
			parentNormalDU = (_parentNormalUVWindow.right - _parentNormalUVWindow.left) / static_cast<float>(_shared->gridSize-1);
		float textureU = (0.5f / static_cast<float>(_shared->textureRes)), 
			parentTextureU = _parentTextureUVWindow.left,
			parentTextureDU = (_parentTextureUVWindow.right - _parentTextureUVWindow.left) / static_cast<float>(_shared->gridSize-1);
		for(int x = 0; x < _shared->gridSize; 
			++x, textureU += textureDUV, parentTextureU += parentTextureDU, 
			normalU += normalDUV, parentNormalU += parentNormalDU, ++vidx)
		{
			ChunkVertex* landVert = _mesh->get_verts()->extract<ChunkVertex>(vidx);
			landVert->textureUV = math::Vector4f(textureU, textureV, parentTextureU, parentTextureV);
			landVert->normalUV = math::Vector4f(normalU, normalV, parentNormalU, parentNormalV);
			if(hasWaterAndIsRoot)
			{
				ChunkWaterL0Vertex* waterVert = _waterMesh->get_verts()->extract<ChunkWaterL0Vertex>(vidx);
				waterVert->parentTextureUV = math::Vector2f(parentTextureU, parentTextureV);
				waterVert->parentNormalUV = math::Vector2f(parentNormalU, parentNormalV);
			}
		}
	}
}

void transfer_uvs(ChunkVertex* sourceVert, ChunkVertex* targetVert)
{
	targetVert->textureUV = sourceVert->textureUV;
	targetVert->normalUV = sourceVert->normalUV;
}

void transfer_uvs(ChunkWaterL0Vertex* sourceVert, ChunkWaterL0Vertex* targetVert)
{
	targetVert->parentTextureUV = sourceVert->parentTextureUV;
	targetVert->parentNormalUV = sourceVert->parentNormalUV;
}

void ChunkLOD::load_geometry_from_thread_data()
{
	using namespace scene;
	assert(!_mesh);
	_mesh.reset(new Geometry());
	_mesh->set_flag(Geometry::Flags::CastShadows);
	_mesh->set_transform(_rootTransform);
	Material::ptr material(new Material());
	material->set_effect(_shader);
	_mesh->set_material(material);
	glbase::VertexSet::ptr verts(new glbase::VertexSet(_vertexSpec, _shared->gridSize * _shared->gridSize + _shared->gridSize * 4));
	_mesh->set_verts(verts);
	glbase::TriangleSet::ptr tris(new glbase::TriangleSet(glbase::TrianglePrimitiveType::TRIANGLE_STRIP));
	_mesh->set_tris(tris);
	create_verts();

	assert(!_waterMesh);
	bool hasWater = _buildThreadData->has_water();
	if(hasWater)
	{
		_waterMesh.reset(new Geometry());
		_waterMesh->set_transform(_rootTransform);
		Material::ptr waterMaterial(new Material());
		waterMaterial->set_effect(is_root()? _waterL0Shader : _waterShader);
		if(is_root())
		{
			waterMaterial->set_parameter("ParentColourTexture", _parentColourTexture);
			waterMaterial->set_parameter("ParentNormalTexture", _parentNormalTexture);
		}
		waterMaterial->set_parameter("PlanetCenterOffset", math::Vector3f(_rootTransform->transform().getColumnVector(3)));

		_waterMesh->set_material(waterMaterial);
		glbase::VertexSet::ptr waterVerts(new glbase::VertexSet(is_root()? _vertexWaterL0Spec : _vertexWaterSpec, 
			_shared->gridSize * _shared->gridSize + _shared->gridSize * 4));
		_waterMesh->set_verts(waterVerts);
		_waterMesh->set_tris(tris);

		if(is_root())
			create_water_verts_root();
		else
			create_water_verts_not_root();
	}
	// calculate uvs
	update_UVs();

	int skirtStartIndex = (int)(_shared->gridSize * _shared->gridSize);

	Transform::matrix4_type rootTransform = _rootTransform->transform();
	Transform::matrix4_type rootTransformInverse = rootTransform.inverse(); // store this
	// create skirt
	float skirtHeightScale = _localBounds.extents().length() / _shared->gridSize; 
	create_skirt_verts<ChunkVertex>(skirtStartIndex, verts, skirtHeightScale, 
		rootTransform, rootTransformInverse, tris, (int)_shared->gridSize);
	update_skirt_UVs_from_edge_UVs<ChunkVertex>(verts, (int)_shared->gridSize);

	if(hasWater)
	{
		if(is_root())
		{
			create_skirt_verts<ChunkWaterL0Vertex>(skirtStartIndex, _waterMesh->get_verts(), skirtHeightScale, 
				rootTransform, rootTransformInverse, glbase::TriangleSet::ptr(), (int)_shared->gridSize);
			update_skirt_UVs_from_edge_UVs<ChunkWaterL0Vertex>(_waterMesh->get_verts(), (int)_shared->gridSize);
		}
		else
			create_skirt_verts<ChunkWaterVertex>(skirtStartIndex, _waterMesh->get_verts(), skirtHeightScale,
			rootTransform, rootTransformInverse, glbase::TriangleSet::ptr(), (int)_shared->gridSize);
	}

	// create tri-strip for verts
	for(int y = 0, idx = 0; y < _shared->gridSize - 1; ++y)
	{
		for(int x = 0; x < _shared->gridSize; ++x, ++idx)
		{
			if(x == 0)
			{
				tris->push_back(idx);
			}
			tris->push_back(idx);
			tris->push_back((glbase::TriangleSet::value_type)(idx + _shared->gridSize));
		}
		if(y != _shared->gridSize - 2)
		{
			tris->push_back((glbase::TriangleSet::value_type)((idx - 1) + _shared->gridSize));
		}
	}

	verts->sync_all();
	tris->sync_all();
	if(_waterMesh)
		_waterMesh->get_verts()->sync_all();

	// update the bounding box 
	_localBounds.expand(_mesh->get_aabb());

	// create normal texture
	_normalTexture.reset(new glbase::Texture());
	_normalTexture->create2D(GL_TEXTURE_2D, (GLuint)_buildThreadData->get_doubled_grid_size(), (GLuint)_buildThreadData->get_doubled_grid_size(), 3, 
		GL_RGB, GL_UNSIGNED_BYTE, _buildThreadData->get_normal(0, 0), GL_RGB, true);

	material->set_parameter("NormalTexture", _normalTexture);

	create_colour_texture();
	material->set_parameter("ColourTexture", _colourTexture);

	material->set_parameter("ParentNormalTexture", _parentNormalTexture);
	material->set_parameter("ParentColourTexture", _parentColourTexture);

	material->set_parameter("BlendFactor", 0.0f);

	update_parent_bounds();

	_buildingState = BuildingState::GeneratingMips;
}

size_t ChunkLOD::get_vert_idx(size_t x, size_t y) const
{
	return y * _shared->gridSize + x;
}

void ChunkLOD::update_children(const Vector3f& localCameraPos, const math::Vector3f& predictedCameraPos, const Errors& childErrors)
{
	assert(_state.get_state() >= State::PrepareChildren);
	for(size_t idx = 0; idx < 4; ++idx)
		_children[idx]->update(localCameraPos, predictedCameraPos, childErrors);
}

bool ChunkLOD::show_children()
{
	check_children_built();

	if(!are_children_built())
		return false;
	for(size_t idx = 0; idx < 4; ++idx)
	{
		bool res = _children[idx]->set_state(State::Shown);
		assert(res);
	}

	return true;
}

const math::AABBf& ChunkLOD::get_local_bounds() const
{
	return _localBounds;
}

void ChunkLOD::update_parent_bounds()
{
	if(_parent != NULL)
	{
		_parent->_localBounds.expand(_localBounds);
		_parent->update_parent_bounds();
	}
}

bool ChunkLOD::hide_children()
{
	for(size_t idx = 0; idx < 4; ++idx)
	{
		bool res = _children[idx]->set_state(State::Prepared);
		assert(res);
	}
	return true;
}

bool ChunkLOD::create_geometry()
{
	check_built();

	if(_buildingState == BuildingState::Built)
		return true;
	if(_buildingState != BuildingState::NotBuilding)
		return false;

	assert(!_mesh);
	assert(!_waterMesh);
	assert(!are_children_created());

	_createChunkProfileBlock = utils::Profiler::start_block(CREATE_GEOMETRY_PROFILE, 50);

	_buildingState = BuildingState::Building;

	assert(!_buildThreadData);
	_buildThreadData.reset(new ChunkLODThreadData(_shared->heightData, _shared->textureData));
	if(_parent != NULL)
	{
		_buildThreadData->init_child(math::Matrix4d(_rootTransform->transform()), 
			(int)_shared->gridSize, _level, _shared->radius, true, _shared->waterRadius);
		for(int y = (_parentArea.top)*2-1, idx = 0; y <= (_parentArea.bottom)*2+1; ++y)
		{
			for(int x = (_parentArea.left)*2-1; x <= (_parentArea.right)*2+1; ++x, ++idx)
			{	
				_buildThreadData->set_parent_pos(x - _parentArea.left*2, y - _parentArea.top*2,
					_parent->_buildThreadData->get_child_pos(x, y));
				_buildThreadData->set_parent_parent_pos(x - _parentArea.left*2, y - _parentArea.top*2,
					_parent->_buildThreadData->get_child_parent_pos(x, y));
			}
		}
	}
	else
	{
		_buildThreadData->init_parent(math::Matrix4d(_rootTransform->transform()), 
			_sourcePts, (int)_shared->gridSize, _level, _shared->radius, true, _shared->waterRadius);
	}

	DataDistributeThread::get_instance()->queue_data(_buildThreadData);

	return false;
}

bool ChunkLOD::show()
{
	using namespace scene;

	assert(!are_children_shown());
	//assert(_mesh->parent() == NULL);
	assert(_mesh);
	//assert(_waterMesh->parent() == NULL);
	assert(!_buildThreadData->has_water() || _waterMesh);

	//_rootTransform->insert(_mesh);
	_shared->geometry->insert(_mesh);
	//_shared->geometry->insert(_skirt);
	//_rootTransform->insert(_waterMesh);
	if(_waterMesh)
		_shared->geometry->insert(_waterMesh);

	return true;
}

bool ChunkLOD::create_children()
{
	float currentResolutionSq = _localBounds.extents().length() / (float)_shared->gridSize;
	if(currentResolutionSq < _shared->maxResolution)
		return false;
	int center = (int)(_shared->gridSize - 1) / 2;
	_children[Quadrants::TOP_LEFT].reset(new ChunkLOD(math::Rectanglei(0, center, center, 0),	
		math::Rectanglef(0.0f, 0.5f, 0.5f, 0.0f),
		this));

	_children[Quadrants::TOP_RIGHT].reset(new ChunkLOD(math::Rectanglei(center, center, (int)_shared->gridSize-1, 0), 
		math::Rectanglef(0.5f, 0.5f, 1.0f, 0.0f),
		this));

	_children[Quadrants::BOTTOM_RIGHT].reset(new ChunkLOD(math::Rectanglei(center, (int)_shared->gridSize-1, (int)_shared->gridSize-1, center),
		math::Rectanglef(0.5f, 1.0f, 1.0f, 0.5f),
		this));

	_children[Quadrants::BOTTOM_LEFT].reset(new ChunkLOD(math::Rectanglei(0, (int)_shared->gridSize-1, center, center),
		math::Rectanglef(0.0f, 1.0f, 0.5f, 0.5f),
		this));

	return true;
}

bool ChunkLOD::show_children_and_hide()
{
	if(!show_children())
		return false;

	hide();

	return true;
}

bool ChunkLOD::hide_children_and_show()
{
	if(!hide_children())
	{
		assert(false);
		return false;
	}

	show();

	return true;
}

bool ChunkLOD::delete_children()
{
	//assert(_state == State::Built);
	for(size_t idx = 0; idx < 4; ++idx)
	{
		ChunkLOD::ptr child = _children[idx];
		assert(child);
		// if the child is currently being built then this function will give it 
		// to the chunk removal manager to delete
		//delete_chunk(child);
		_children[idx].reset();
	}

	return true;
}

bool ChunkLOD::hide()
{
	assert(_mesh);
	assert(!_buildThreadData->has_water() || _waterMesh);
	//_shared->geometry->erase(_skirt);

	if(_batched)
	{
		_chunkLandCache->remove(this);
		if(_buildThreadData->has_water())
			_chunkWaterCache->remove(this);
		_batched = false;
	}
	else
	{
		_shared->geometry->erase(_mesh);
		if(_buildThreadData->has_water())
			_shared->geometry->erase(_waterMesh);
	}

	return true;
}

bool ChunkLOD::delete_geometry()
{
	assert(!are_children_created());
	abort_build();
	assert(!_mesh || !_shared->geometry->contains(_mesh));
	delete_geometry_data();
	_buildingState = BuildingState::NotBuilding;
	return true;
}

void ChunkLOD::delete_geometry_data()
{
	_mesh.reset();
	//_skirt.reset();
	_waterMesh.reset();
}


bool ChunkLOD::is_shown_or_children_shown() const
{
	return _state.get_state() >= State::Shown;
}

bool ChunkLOD::is_shown() const 
{ 
	return _state.get_state() == State::Shown; 
}

size_t ChunkLOD::get_curr_max_depth() const
{
	size_t depth = 0, maxDepth = 0;
	get_curr_max_depth_int(depth, maxDepth);
	return maxDepth;
}

void ChunkLOD::get_curr_max_depth_int(size_t& depth, size_t& maxDepth) const
{
	++depth;
	maxDepth = std::max(depth, maxDepth);
	if(are_children_created())
	{
		for(size_t idx = 0; idx < 4; ++idx)
			_children[idx]->get_curr_max_depth_int(depth, maxDepth);
	}
	--depth;
}

bool ChunkLOD::are_children_created() const
{
	return (bool)_children[0];
}

bool ChunkLOD::are_children_shown() const
{
	if(!are_children_created())
		return false;
	for(size_t idx = 0; idx < 4; ++idx)
	{
		if(!_children[idx]->is_shown())
			return false;
	}
	return true;
}

void ChunkLOD::check_children_built()
{
	if(!are_children_created())
		return;

	for(size_t idx = 0; idx < 4; ++idx)
	{
		_children[idx]->check_built();
	}
}

bool ChunkLOD::are_children_built() const
{
	if(!are_children_created())
		return false;

	for(size_t idx = 0; idx < 4; ++idx)
	{
		if(!_children[idx]->is_built())
			return false;
	}

	return true;
}

bool ChunkLOD::is_root() const
{
	return _parent == NULL;
}

// if we think we are building and the buildThreadData says it is not building
// then we assume that we are now built
void ChunkLOD::check_built()
{
	switch(_buildingState)
	{
	case BuildingState::Building:
		if(!_buildThreadData->is_finished_build())
			return ;
		if(_buildThreadData->is_aborted())
		{
			_buildingState = BuildingState::NotBuilding;
			return ;
		}
		_buildingState = BuildingState::CreatingGeometry;
		;
	case BuildingState::CreatingGeometry:
		load_geometry_from_thread_data();
		_buildingState = BuildingState::GeneratingMips;
		return ;
	case BuildingState::GeneratingMips:
		_normalTexture->generate_mipmaps();
		_colourTexture->generate_mipmaps();

		utils::Profiler::end_block(_createChunkProfileBlock);

		_buildingState = BuildingState::Built;
		return ;
	case BuildingState::Built:
		return ;
	}
}

void ChunkLOD::abort_build()
{
	if(_buildThreadData)
		_buildThreadData->set_abort_build();
	_buildThreadData.reset();
	_buildingState = BuildingState::NotBuilding;
}

void ChunkLOD::wait_for_build()
{
	if(_buildingState == BuildingState::Building)
	{
		_buildThreadData->wait_for_build();
		_buildingState = (_buildThreadData->is_aborted()? BuildingState::NotBuilding : BuildingState::CreatingGeometry);
	}
}

bool ChunkLOD::is_built() const
{
	return _buildingState == BuildingState::Built;
}

bool ChunkLOD::is_building() const
{
	return _buildingState != BuildingState::Built && _buildingState != BuildingState::NotBuilding;
}

bool ChunkLOD::is_near() const
{
	return true;//_level > 5;
}

void ChunkLOD::create_colour_texture()
{
	// create colour texture
	_colourTexture.reset(new glbase::Texture());
	_colourTexture->create2D(GL_TEXTURE_2D, (GLuint)_shared->textureRes, (GLuint)_shared->textureRes, 4, 
		GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_RGBA, true);
	_colourTexture->set_wrap(glbase::WrapMode::ClampToEdge, glbase::WrapMode::ClampToEdge);

	_shared->texturingFBO->AttachTexture(_colourTexture->handle(), GL_COLOR_ATTACHMENT0);
	assert(_shared->texturingFBO->IsValid());

	_shared->texturingFBO->Bind();
	_shared->texturingFBO->BindDrawBuffers();

	glViewport(0, 0, (GLsizei)_shared->textureRes, (GLsizei)_shared->textureRes);

	//glMatrixMode(GL_PROJECTION);
	//glLoadIdentity();

	//glMatrixMode(GL_MODELVIEW);
	//glLoadIdentity();

	//transform::Camera camera;
	//camera.set_type(transform::ProjectionType::Orthographic);
	//camera.set_render_area(math::Rectanglef(0, static_cast<float>(_shared->gridSize * 2 - 1), 
	//	static_cast<float>(_shared->gridSize * 2 - 1), 0));
	//Viewport::ptr viewport(new Viewport(0, 0, (unsigned int)_shared->textureRes, (unsigned int)_shared->textureRes));
	//camera.set_viewport(viewport);
	//camera.set_near_plane(Camera::float_type(-1));
	//camera.set_far_plane(Camera::float_type(1));

	//float splatScale = 5000.0f;// / pow(2.0f, static_cast<float>(_level));

	struct TextureCreationVert
	{
		math::Vector2f pos;
		math::Vector2f uv;
		float weight;
	};

	assert(sizeof(TextureCreationVert) == _shared->textureCreationVerts->spec()->vertexSize());

	int splatGridSize = _buildThreadData->get_doubled_grid_size();
	//double offsetU = ::floor(_textureUVWindow.left);
	double startU = _textureUVWindow.left /*- offsetU*/;
	//double offsetV = ::floor(_textureUVWindow.top);
	double startV = _textureUVWindow.top /*- offsetV*/;
	double du = (_textureUVWindow.right - _textureUVWindow.left) / static_cast<double>(splatGridSize - 1);
 	// stupid texturing bodge because we only advance textureSize-1 pixels
 	//du *= (_shared->textureRes-1) / static_cast<double>(_shared->textureRes);
 	double dv = (_textureUVWindow.bottom - _textureUVWindow.top) / static_cast<double>(splatGridSize - 1);
 	// stupid texturing bodge because we only advance textureSize-1 pixels
 	//dv *= (_shared->textureRes-1) / static_cast<double>(_shared->textureRes);

	for(int tIdx = 0; tIdx < _shared->textureData->get_textures_count(); ++tIdx)
	{
		PlanetTexture::ptr planetTexture = _shared->textureData->get_texture(tIdx);

		_shared->textureCreationMaterial->set_parameter("ColourTexture", planetTexture->get_texture());

		double v = startV;
		for(int y = 0; y < splatGridSize; ++y, v += dv)
		{
			//glBegin(GL_TRIANGLE_STRIP);
			double u = startU;
			for(int x = 0; x < splatGridSize; ++x, u += du)
			{
				TextureCreationVert* vert = _shared->textureCreationVerts->extract<TextureCreationVert>(y * splatGridSize + x);

				vert->pos = math::Vector2f(x, /*(splatGridSize - 1) -*/ y);
				vert->weight = _buildThreadData->get_texture_weight(x, y, tIdx);
				vert->uv = math::Vector2f(u, v);
			//	//float u = x * splatScale;
			//	draw_texture_vertex(static_cast<float>(u), static_cast<float>(v), tWeight0, x, (int)(_shared->gridSize * 2 - 1) - y);
			//	draw_texture_vertex(static_cast<float>(u), static_cast<float>(v + dv), tWeight1, x, (int)(_shared->gridSize * 2 - 2) - y);
			}
			//glEnd();
		}
		_shared->textureCreationVerts->sync_all();

		_shared->textureCreationMaterial->bind();
		glbase::VideoMemoryManager::load_buffers(_shared->textureCreationTris, _shared->textureCreationVerts);
		glbase::VideoMemoryManager::render_current();
		glbase::VideoMemoryManager::unbind_buffers();
		scene::Material::unbind();
	}

	FramebufferObject::Disable();
}

void ChunkLOD::load_land_verts(BatchableLandVertex* targetVerts, float UVz)
{
	glbase::VertexSet::ptr verts = _mesh->get_verts();

	ChunkVertex* pVert = verts->extract<ChunkVertex>(0);
	for(size_t idx = 0; idx < verts->get_count(); ++idx, ++pVert, ++targetVerts)
	{
		targetVerts->position = pVert->pos;
		targetVerts->textureUV.x = pVert->textureUV.x;
		targetVerts->textureUV.y = pVert->textureUV.y;
		targetVerts->textureUV.z = UVz;
		targetVerts->normalUV.x = pVert->normalUV.x;
		targetVerts->normalUV.y = pVert->normalUV.y;
		targetVerts->normalUV.z = UVz;
	}
}

glbase::TriangleSet::value_type ChunkLOD::load_land_indices(const glbase::TriangleSet::ptr& targetTris, glbase::TriangleSet::value_type offset, bool stitchStart)
{
	glbase::TriangleSet::ptr sourceTris = _mesh->get_tris();
	if(stitchStart)
		targetTris->push_back((*sourceTris)[0] + offset);
	for(size_t idx = 0; idx < sourceTris->get_count(); ++idx)
	{
		targetTris->push_back((*sourceTris)[idx] + offset);
	}
	return (*sourceTris)[sourceTris->get_count()-1] + offset;
}

glbase::Texture::ptr ChunkLOD::get_colour_texture() const
{
	return _colourTexture;
}

glbase::Texture::ptr ChunkLOD::get_normal_texture() const
{
	return _normalTexture;
}

void ChunkLOD::load_water_verts(BatchableWaterVertex* targetVerts)
{
	glbase::VertexSet::ptr verts = _waterMesh->get_verts();

	if(is_root())
	{
		ChunkWaterL0Vertex* pVert = verts->extract<ChunkWaterL0Vertex>(0);
		for(size_t idx = 0; idx < verts->get_count(); ++idx, ++pVert, ++targetVerts)
		{
			targetVerts->position = pVert->pos;
		}
	}
	else
	{
		assert(sizeof(ChunkWaterVertex) == sizeof(BatchableWaterVertex));
		ChunkWaterVertex* pVert = verts->extract<ChunkWaterVertex>(0);
		::memcpy(targetVerts, pVert, verts->get_local_buffer_size_bytes());
	}
}

glbase::TriangleSet::value_type ChunkLOD::load_water_indices(const glbase::TriangleSet::ptr& targetTris, glbase::TriangleSet::value_type offset, bool stitchStart)
{
	glbase::TriangleSet::ptr sourceTris = _waterMesh->get_tris();
	if(stitchStart)
		targetTris->push_back((*sourceTris)[0] + offset);
	for(size_t idx = 0; idx < sourceTris->get_count(); ++idx)
	{
		targetTris->push_back((*sourceTris)[idx] + offset);
	}
	return (*sourceTris)[sourceTris->get_count()-1] + offset;
}

void ChunkLOD::static_init()
{
	using namespace glbase;

	_shader.reset(new effect::Effect());
	_shader->load("../Data/Shaders/lod_chunk.xml");
	CHECK_OPENGL_ERRORS;
	_waterShader.reset(new effect::Effect());
	_waterShader->load("../Data/Shaders/lod_chunk_water.xml");
	CHECK_OPENGL_ERRORS;
	_waterL0Shader.reset(new effect::Effect());
	_waterL0Shader->load("../Data/Shaders/lod_chunkL0_water.xml");
	CHECK_OPENGL_ERRORS;


	_vertexSpec.reset(new VertexSpec());

	_vertexSpec->append(VertexData::PositionData, 0, sizeof(float), 3, VertexElementType::Float);
	_vertexSpec->append(VertexData::TexCoord0, 1, sizeof(float), 3, VertexElementType::Float);
	_vertexSpec->append(VertexData::TexCoord1, 2, sizeof(float), 4, VertexElementType::Float);
	_vertexSpec->append(VertexData::TexCoord2, 3, sizeof(float), 4, VertexElementType::Float);
	assert(sizeof(ChunkVertex) == _vertexSpec->vertexSize());

	_vertexWaterSpec.reset(new VertexSpec());
	_vertexWaterSpec->append(VertexData::PositionData, 0, sizeof(float), 3, VertexElementType::Float);
	assert(sizeof(ChunkWaterVertex) == _vertexWaterSpec->vertexSize());

	_vertexWaterL0Spec.reset(new VertexSpec());
	_vertexWaterL0Spec->append(VertexData::PositionData, 0, sizeof(float), 3, VertexElementType::Float);
	_vertexWaterL0Spec->append(VertexData::TexCoord0, 1, sizeof(float), 3, VertexElementType::Float);
	_vertexWaterL0Spec->append(VertexData::TexCoord1, 2, sizeof(float), 4, VertexElementType::Float);
	assert(sizeof(ChunkWaterL0Vertex) == _vertexWaterL0Spec->vertexSize());

	_batchLandVertexSpec.reset(new VertexSpec());
	_batchLandVertexSpec->append(VertexData::PositionData, 0, sizeof(float), 3, VertexElementType::Float);
	_batchLandVertexSpec->append(VertexData::TexCoord0, 1, sizeof(float), 3, VertexElementType::Float);
	_batchLandVertexSpec->append(VertexData::TexCoord1, 2, sizeof(float), 3, VertexElementType::Float);
	assert(sizeof(BatchableLandVertex) == _batchLandVertexSpec->vertexSize());

	_batchWaterVertexSpec.reset(new VertexSpec());
	_batchWaterVertexSpec->append(VertexData::PositionData, 0, sizeof(float), 3, VertexElementType::Float);
	assert(sizeof(BatchableWaterVertex) == _batchWaterVertexSpec->vertexSize());


	BatchLandCache::load_shaders();
	BatchWaterCache::load_shaders();
}

void ChunkLOD::static_release()
{
	_shader.reset();
	_waterShader.reset();
	_waterL0Shader.reset();
	_vertexSpec.reset();
	_vertexWaterSpec.reset();
	_vertexWaterL0Spec.reset();
	_batchLandVertexSpec.reset();
	_batchWaterVertexSpec.reset();
	BatchLandCache::release_shaders();
	BatchWaterCache::release_shaders();

	for(auto cItr = gChunksToDelete.begin(); cItr != gChunksToDelete.end(); ++cItr)
	{
		ChunkLOD::ptr chunk = *cItr;
		chunk->abort_build();
	}
	for(auto cItr = gChunksToDelete.begin(); cItr != gChunksToDelete.end(); ++cItr)
	{
		ChunkLOD::ptr chunk = *cItr;
		chunk->wait_for_build();
	}
	gChunksToDelete.clear();
}

const std::string ChunkLOD::CREATE_GEOMETRY_PROFILE("ChunkLOD::create_geometry");

effect::Effect::ptr ChunkLOD::_waterL0Shader;
effect::Effect::ptr ChunkLOD::_waterShader;
effect::Effect::ptr ChunkLOD::_shader;
glbase::VertexSpec::ptr ChunkLOD::_vertexSpec;
glbase::VertexSpec::ptr ChunkLOD::_vertexWaterSpec;
glbase::VertexSpec::ptr ChunkLOD::_vertexWaterL0Spec;
glbase::VertexSpec::ptr ChunkLOD::_batchLandVertexSpec;
glbase::VertexSpec::ptr ChunkLOD::_batchWaterVertexSpec;

// ---------------------------------------------------------------------------------
// BatchLandCache 
// ---------------------------------------------------------------------------------

BatchLandCache::BatchLandCache(size_t batchableCountPerBatch, 
					   size_t batchableTextureSize, 
					   size_t batchableNormalTextureSize, 
					   size_t batchableVertexCount,
					   const glbase::VertexSpec::ptr& vertexSpec, 
					   const scene::transform::Transform::ptr& trans,
					   const render::GeometrySet::ptr& geometrySet,
					   const FramebufferObject::ptr& textureTransferFBO)
	: _batchableCountPerBatch(batchableCountPerBatch),
	_batchableTextureSize(batchableTextureSize),
	_batchableNormalTextureSize(batchableNormalTextureSize),
	_batchableVertexCount(batchableVertexCount),
	_vertexSpec(vertexSpec),
	_trans(trans),
	_geometrySet(geometrySet),
	_textureTransferFBO(textureTransferFBO)
{
	// check batchableCountPerSide is a power of 2
	assert((batchableCountPerBatch & (batchableCountPerBatch - 1)) == 0);

}

void BatchLandCache::add( BatchableLand* batchable )
{
	_addList.push_back(batchable);
}

void BatchLandCache::remove( BatchableLand* batchable )
{
	_removeList.push_back(batchable);
}

void BatchLandCache::update()
{
	BatchSet updateList;
	// process the remove list and remove batchables from their 
	// batches
	for(size_t idx = 0; idx < _removeList.size(); ++idx)
	{
		BatchableLand* batchable = _removeList[idx];
		BatchMap::iterator itr = _batchMap.find(batchable);
		assert(itr != _batchMap.end());

		Batch::ptr batch = itr->second;
		batch->remove(batchable);
		_batchMap.erase(itr);
		updateList.insert(batch);
	}
	_removeList.clear();
	// process the add list and add batchables to appropriate batches
	for(size_t idx = 0; idx < _addList.size(); ++idx)
	{
		BatchableLand* batchable = _addList[idx];
		bool addedToBatch = false;
		for(BatchSet::iterator bItr = _batches.begin(); bItr != _batches.end(); ++bItr)
		{
			Batch::ptr batch = *bItr;
			if(batch->get_free_space() != 0)
			{
				batch->add(batchable);
				_batchMap.insert(std::make_pair(batchable, batch));
				updateList.insert(batch);
				addedToBatch = true;
				break;
			}
		}
		if(!addedToBatch)
		{
			Batch::ptr newBatch(new Batch(this));
			_batches.insert(newBatch);
			newBatch->add(batchable);
			_batchMap.insert(std::make_pair(batchable, newBatch));
			updateList.insert(newBatch);
		}
	}
	_addList.clear();
	for(BatchSet::iterator bItr = updateList.begin(); bItr != updateList.end(); ++bItr)
	{
		(*bItr)->update();
	}
}

void BatchLandCache::load_shaders()
{
	_batchLandShader.reset(new effect::Effect());
	_batchLandShader->load("../Data/Shaders/lod_chunk_batch.xml");
	CHECK_OPENGL_ERRORS;
}

void BatchLandCache::release_shaders()
{
	_batchLandShader.reset();
}

effect::Effect::ptr BatchLandCache::_batchLandShader;

// ---------------------------------------------------------------------------------
// BatchLandCache::Batch
// ---------------------------------------------------------------------------------

BatchLandCache::Batch::Batch(BatchLandCache* owner) 
	: _geometry(new scene::Geometry()),
	_colorTexture(new glbase::Texture()),
	_normalTexture(new glbase::Texture()),
	_owner(owner)
{
	_colorTexture->create3D(GL_TEXTURE_2D_ARRAY, (GLuint)owner->_batchableTextureSize, (GLuint)owner->_batchableTextureSize, 
		(GLuint)owner->_batchableCountPerBatch, 4, GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_RGBA8, true);	
	_normalTexture->create3D(GL_TEXTURE_2D_ARRAY, (GLuint)owner->_batchableNormalTextureSize, (GLuint)owner->_batchableNormalTextureSize, 
		(GLuint)owner->_batchableCountPerBatch, 4, GL_RGBA, GL_UNSIGNED_BYTE, NULL, GL_RGBA8, true);

	scene::Material::ptr material(new scene::Material());
	material->set_effect(_batchLandShader);
	material->set_parameter("ColourTexture", _colorTexture);
	material->set_parameter("NormalTexture", _normalTexture);
	_geometry->set_material(material);
	_geometry->set_flag(Geometry::Flags::CastShadows);

	_geometry->set_transform(_owner->_trans);

	glbase::VertexSet::ptr verts(new glbase::VertexSet(_owner->_vertexSpec, _owner->_batchableVertexCount * _owner->_batchableCountPerBatch, GL_STREAM_DRAW));

	//scene::TriangleSet::ptr tris(new scene::TriangleSet());
	_geometry->set_verts(verts);
	//_geometry->set_tris(tris);
	for(index_type idx = 0; idx < _owner->_batchableCountPerBatch; ++idx)
	{
		_freeSpaces.insert(idx);
	}
}

void BatchLandCache::Batch::add( BatchableLand* batchable )
{
	assert(!_freeSpaces.empty());

	index_type freeSpace = *_freeSpaces.begin();
	_freeSpaces.erase(_freeSpaces.begin());

	size_t vertexOffset = freeSpace * _owner->_batchableVertexCount;
	BatchableLandVertex* vertPtr = _geometry->get_verts()->extract<BatchableLandVertex>(vertexOffset);
	batchable->load_land_verts(vertPtr, static_cast<float>(freeSpace));
	_geometry->get_verts()->sync_range(vertexOffset, vertexOffset + _owner->_batchableVertexCount);
	_chunks.insert(std::make_pair(batchable, BatchableRecord(freeSpace, vertexOffset)));

	// copy colour texture
	_owner->_textureTransferFBO->Bind();
	_owner->_textureTransferFBO->AttachTexture(batchable->get_colour_texture()->handle(), GL_COLOR_ATTACHMENT0);
	assert(_owner->_textureTransferFBO->IsValid());
	//glViewport(0, 0, _owner->_batchableTextureSize, _owner->_batchableTextureSize);
	glReadBuffer(GL_COLOR_ATTACHMENT0_EXT);
	_colorTexture->bind();
	glCopyTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, (GLint)freeSpace, 0, 0, (GLsizei)_owner->_batchableTextureSize, (GLsizei)_owner->_batchableTextureSize);
	_colorTexture->unbind();
	// copy normal texture
	_owner->_textureTransferFBO->AttachTexture(batchable->get_normal_texture()->handle(), GL_COLOR_ATTACHMENT0);
	assert(_owner->_textureTransferFBO->IsValid());
	//glViewport(0, 0, _owner->_batchableNormalTextureSize, _owner->_batchableNormalTextureSize);
	_normalTexture->bind();
	glCopyTexSubImage3D(GL_TEXTURE_2D_ARRAY, 0, 0, 0, (GLint)freeSpace, 0, 0, (GLsizei)_owner->_batchableNormalTextureSize, (GLsizei)_owner->_batchableNormalTextureSize);
	_normalTexture->unbind();

	FramebufferObject::Disable();
}

void BatchLandCache::Batch::remove( BatchableLand* batchable )
{
	BatchableMap::iterator bItr = _chunks.find(batchable);
	_freeSpaces.insert(bItr->second.index);
	_chunks.erase(bItr);
}

void BatchLandCache::Batch::update()
{
	if(_chunks.empty())
	{
		if(_geometry->get_tris())
		{
			_geometry->set_tris(glbase::TriangleSet::ptr());
			_owner->_geometrySet->erase(_geometry);
		}
		return;
	}
	glbase::TriangleSet::ptr tris(new glbase::TriangleSet(glbase::TrianglePrimitiveType::TRIANGLE_STRIP));

	glbase::TriangleSet::value_type lastIndex = 0;
	for(BatchableMap::const_iterator bItr = _chunks.begin(); bItr != _chunks.end(); ++bItr)
	{
		BatchableLand* batchable = bItr->first;
		bool stitch = bItr != _chunks.begin();
		if(stitch)
			tris->push_back(lastIndex);
		lastIndex = batchable->load_land_indices(tris, (glbase::TriangleSet::value_type)bItr->second.vertexIndexOffset, stitch);
	}

	tris->sync_all();

	if(!_geometry->get_tris())
		_owner->_geometrySet->insert(_geometry);
	_geometry->set_tris(tris);
}

// ---------------------------------------------------------------------------------
// BatchWaterCache 
// ---------------------------------------------------------------------------------

BatchWaterCache::BatchWaterCache(size_t batchableCountPerBatch, 
							   size_t batchableVertexCount,
							   const glbase::VertexSpec::ptr& vertexSpec, 
							   const scene::transform::Transform::ptr& trans,
							   const render::GeometrySet::ptr& geometrySet)
							   : _batchableCountPerBatch(batchableCountPerBatch),
							   _batchableVertexCount(batchableVertexCount),
							   _vertexSpec(vertexSpec),
							   _trans(trans),
							   _geometrySet(geometrySet)
{
	// check batchableCountPerSide is a power of 2
	assert((batchableCountPerBatch & (batchableCountPerBatch - 1)) == 0);

}

void BatchWaterCache::add( BatchableWater* batchable )
{
	_addList.push_back(batchable);
}

void BatchWaterCache::remove( BatchableWater* batchable )
{
	_removeList.push_back(batchable);
}

void BatchWaterCache::update()
{
	BatchSet updateList;
	// process the remove list and remove batchables from their 
	// batches
	for(size_t idx = 0; idx < _removeList.size(); ++idx)
	{
		BatchableWater* batchable = _removeList[idx];
		BatchMap::iterator itr = _batchMap.find(batchable);
		assert(itr != _batchMap.end());

		Batch::ptr batch = itr->second;
		batch->remove(batchable);
		_batchMap.erase(itr);
		updateList.insert(batch);
	}
	_removeList.clear();
	// process the add list and add batchables to appropriate batches
	for(size_t idx = 0; idx < _addList.size(); ++idx)
	{
		BatchableWater* batchable = _addList[idx];
		bool addedToBatch = false;
		for(BatchSet::iterator bItr = _batches.begin(); bItr != _batches.end(); ++bItr)
		{
			Batch::ptr batch = *bItr;
			if(batch->get_free_space() != 0)
			{
				batch->add(batchable);
				_batchMap.insert(std::make_pair(batchable, batch));
				updateList.insert(batch);
				addedToBatch = true;
				break;
			}
		}
		if(!addedToBatch)
		{
			Batch::ptr newBatch(new Batch(this));
			_batches.insert(newBatch);
			newBatch->add(batchable);
			_batchMap.insert(std::make_pair(batchable, newBatch));
			updateList.insert(newBatch);
		}
	}
	_addList.clear();
	for(BatchSet::iterator bItr = updateList.begin(); bItr != updateList.end(); ++bItr)
	{
		(*bItr)->update();
	}
}

void BatchWaterCache::load_shaders()
{
	_batchWaterShader.reset(new effect::Effect());
	_batchWaterShader->load("../Data/Shaders/lod_chunk_water_batch.xml");
	CHECK_OPENGL_ERRORS;
}

void BatchWaterCache::release_shaders()
{
	_batchWaterShader.reset();
}

effect::Effect::ptr BatchWaterCache::_batchWaterShader;

// ---------------------------------------------------------------------------------
// BatchWaterCache::Batch
// ---------------------------------------------------------------------------------

BatchWaterCache::Batch::Batch(BatchWaterCache* owner) 
: _geometry(new scene::Geometry()),
_owner(owner)
{
	scene::Material::ptr material(new scene::Material());
	material->set_effect(_batchWaterShader);
	material->set_parameter("PlanetCenterOffset", math::Vector3f(owner->_trans->transform().getColumnVector(3)));
	_geometry->set_material(material);
	_geometry->set_flag(Geometry::Flags::CastShadows);

	_geometry->set_transform(_owner->_trans);

	glbase::VertexSet::ptr verts(new glbase::VertexSet(_owner->_vertexSpec,
		_owner->_batchableVertexCount * _owner->_batchableCountPerBatch, GL_STREAM_DRAW));

	_geometry->set_verts(verts);
	for(index_type idx = 0; idx < _owner->_batchableCountPerBatch; ++idx)
	{
		_freeSpaces.insert(idx);
	}
}

void BatchWaterCache::Batch::add( BatchableWater* batchable )
{
	assert(!_freeSpaces.empty());

	index_type freeSpace = *_freeSpaces.begin();
	_freeSpaces.erase(_freeSpaces.begin());

	size_t vertexOffset = freeSpace * _owner->_batchableVertexCount;
	BatchableWaterVertex* vertPtr = _geometry->get_verts()->extract<BatchableWaterVertex>(vertexOffset);
	batchable->load_water_verts(vertPtr);
	_geometry->get_verts()->sync_range(vertexOffset, vertexOffset + _owner->_batchableVertexCount);
	_chunks.insert(std::make_pair(batchable, BatchableRecord(freeSpace, vertexOffset)));
}

void BatchWaterCache::Batch::remove( BatchableWater* batchable )
{
	BatchableMap::iterator bItr = _chunks.find(batchable);
	_freeSpaces.insert(bItr->second.index);
	_chunks.erase(bItr);
}

void BatchWaterCache::Batch::update()
{
	if(_chunks.empty())
	{
		if(_geometry->get_tris())
		{
			_geometry->set_tris(glbase::TriangleSet::ptr());
			_owner->_geometrySet->erase(_geometry);
		}
		return;
	}
	glbase::TriangleSet::ptr tris(new glbase::TriangleSet(glbase::TrianglePrimitiveType::TRIANGLE_STRIP));

	glbase::TriangleSet::value_type lastIndex = 0;
	for(BatchableMap::const_iterator bItr = _chunks.begin(); bItr != _chunks.end(); ++bItr)
	{
		BatchableWater* batchable = bItr->first;
		bool stitch = bItr != _chunks.begin();
		if(stitch)
			tris->push_back(lastIndex);
		lastIndex = batchable->load_water_indices(tris, (glbase::TriangleSet::value_type)bItr->second.vertexIndexOffset, stitch);
	}

	tris->sync_all();

	if(!_geometry->get_tris())
		_owner->_geometrySet->insert(_geometry);
	_geometry->set_tris(tris);
}

}