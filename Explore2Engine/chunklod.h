#if !defined(__EXPLORE2_CHUNKLOD_H__)
#define __EXPLORE2_CHUNKLOD_H__

#include <functional>

#include "Math/rectangle.hpp"

#include "Scene/geometry.hpp"
#include "Scene/camera.hpp"
//#include "Scene/meshtransform.hpp"

#include "render/scenecontext.hpp"

#include "Misc/atomic.hpp"

#include "heightdataprovider.h"
#include "textureprovider.h"

#include "chunkdatathread.h"

#include "Misc/statemachine.hpp"

//#include "Utils/Profiler.h"

namespace explore2 {;

/*
Optimize cache rendering:
When a chunk has been at 1.0 weight for x amount of time/frames it can
be cached into a single batch with other 1.0 weight chunks.
The batch will consist of a texture containing all batched chunk textures,
a vertex buffer containing all batched chunk verts and an index buffer containing
all chunked indices. 
Each top level chunk has a separate cache.
The batches have their own vertex format, that includes:
vertex position
uv - transformed to fit the batches texture atlas
normalUV - transformed to fit the batches normal texture atlas

When a chunk detects that it is able to be cached it notifies the cache giving a pointer
to itself.
When a chunk is hidden for any reason it notifies the cache that it needs to be removed.
At the end of a top level chunk update, the update function of its cache is called.
The update function will determine 
*/

struct BatchableLandVertex
{
	math::Vector3f position;
	math::Vector3f textureUV;
	math::Vector3f normalUV;
};

struct BatchableWaterVertex
{
	math::Vector3f position;
};

struct BatchableLand
{
	virtual void load_land_verts(BatchableLandVertex* targetVerts, float UVz) = 0;
	virtual glbase::TriangleSet::value_type load_land_indices(const glbase::TriangleSet::ptr& targetTris, 
		glbase::TriangleSet::value_type offset, bool stitchStart) = 0;
	virtual glbase::Texture::ptr get_colour_texture() const = 0;
	virtual glbase::Texture::ptr get_normal_texture() const = 0;
};

struct BatchLandCache
{
	typedef std::shared_ptr< BatchLandCache > ptr;
	struct Batch
	{
		typedef std::shared_ptr<Batch> ptr;

		Batch(BatchLandCache* owner);

		void add(BatchableLand* batchable);
		void remove(BatchableLand* batchable);
		void update();

		const math::AABBf& get_aabb() const { return _aabb; }
		size_t get_free_space() const { return _freeSpaces.size(); }

	private:
		typedef size_t index_type;
		struct BatchableRecord
		{
			BatchableRecord(index_type index_, size_t vertexIndexOffset_)
				: index(index_), vertexIndexOffset(vertexIndexOffset_) {}
			index_type index;
			size_t vertexIndexOffset;
		};
		typedef std::map<BatchableLand*, BatchableRecord> BatchableMap;
		BatchableMap _chunks;
		std::set<index_type> _freeSpaces;
		scene::Geometry::ptr _geometry;
		glbase::Texture::ptr _colorTexture, _normalTexture;
		math::AABBf _aabb;
		BatchLandCache* _owner;
	};

	static void load_shaders();
	static void release_shaders();

	BatchLandCache(size_t batchableCountPerBatch, 
		size_t batchableTextureSize, 
		size_t batchableNormalTextureSize, 
		size_t batchableVertexCount,
		const glbase::VertexSpec::ptr& vertexSpec, 
		const scene::transform::Transform::ptr& trans,
		const render::GeometrySet::ptr& geometrySet,
		const FramebufferObject::ptr& textureTransferFBO);


	void add(BatchableLand* batchable);
	void remove(BatchableLand* batchable);

	void update();
private:
	typedef std::set<Batch::ptr> BatchSet;
	BatchSet _batches;
	std::vector<BatchableLand*> _addList, _removeList;
	typedef std::map<BatchableLand*, Batch::ptr> BatchMap;
	BatchMap _batchMap;
	size_t _batchableCountPerBatch, _batchableTextureSize, 
		_batchableNormalTextureSize, _batchableVertexCount;
	scene::transform::Transform::ptr _trans;
	glbase::VertexSpec::ptr _vertexSpec;
	render::GeometrySet::ptr _geometrySet;
	FramebufferObject::ptr _textureTransferFBO;

	static effect::Effect::ptr _batchLandShader;
};

struct BatchableWater
{
	virtual void load_water_verts(BatchableWaterVertex* targetVerts) = 0;
	virtual glbase::TriangleSet::value_type load_water_indices(const glbase::TriangleSet::ptr& targetTris, 
		glbase::TriangleSet::value_type offset, bool stitchStart) = 0;
};

struct BatchWaterCache
{
	typedef std::shared_ptr< BatchWaterCache > ptr;
	struct Batch
	{
		typedef std::shared_ptr<Batch> ptr;

		Batch(BatchWaterCache* owner);

		void add(BatchableWater* batchable);
		void remove(BatchableWater* batchable);
		void update();

		const math::AABBf& get_aabb() const { return _aabb; }
		size_t get_free_space() const { return _freeSpaces.size(); }

	private:
		typedef size_t index_type;
		struct BatchableRecord
		{
			BatchableRecord(index_type index_, size_t vertexIndexOffset_)
				: index(index_), vertexIndexOffset(vertexIndexOffset_) {}
			index_type index;
			size_t vertexIndexOffset;
		};
		typedef std::map<BatchableWater*, BatchableRecord> BatchableMap;
		BatchableMap _chunks;
		std::set<index_type> _freeSpaces;
		scene::Geometry::ptr _geometry;
		math::AABBf _aabb;
		BatchWaterCache* _owner;
	};

	static void load_shaders();
	static void release_shaders();

	BatchWaterCache(size_t batchableCountPerBatch, 
		size_t batchableVertexCount,
		const glbase::VertexSpec::ptr& vertexSpec, 
		const scene::transform::Transform::ptr& trans,
		const render::GeometrySet::ptr& geometrySet);


	void add(BatchableWater* batchable);
	void remove(BatchableWater* batchable);

	void update();
private:
	typedef std::set<Batch::ptr> BatchSet;
	BatchSet _batches;
	std::vector<BatchableWater*> _addList, _removeList;
	typedef std::map<BatchableWater*, Batch::ptr> BatchMap;
	BatchMap _batchMap;
	size_t _batchableCountPerBatch, _batchableVertexCount;
	scene::transform::Transform::ptr _trans;
	glbase::VertexSpec::ptr _vertexSpec;
	render::GeometrySet::ptr _geometrySet;

	static effect::Effect::ptr _batchWaterShader;
};

struct ChunkVertex
{
	ChunkVertex(const math::Vector3f& pos_, const math::Vector3f& posParent_, 
		const math::Vector4f& textureUV_, 
		const math::Vector4f& normalUV_) 
		: pos(pos_), posParent(posParent_), textureUV(textureUV_), normalUV(normalUV_) {}
	ChunkVertex(const math::Vector3f& pos_ = math::Vector3f(), 
		const math::Vector3f& posParent_ = math::Vector3f()) 
		: pos(pos_), posParent(posParent_) {}

	math::Vector3f pos;
	math::Vector3f posParent;
	math::Vector4f textureUV;
	math::Vector4f normalUV;
};	

struct ChunkWaterVertex
{
	ChunkWaterVertex(const math::Vector3f& pos_ = math::Vector3f()) 
		: pos(pos_) {}

	math::Vector3f pos;
};	

struct ChunkWaterL0Vertex
{
	ChunkWaterL0Vertex(const math::Vector3f& pos_, const math::Vector3f& posParent_, 
		const math::Vector2f& parentTextureUV_, 
		const math::Vector2f& parentNormalUV_) 
		: pos(pos_), posParent(posParent_), parentTextureUV(parentTextureUV_), parentNormalUV(parentNormalUV_) {}
	ChunkWaterL0Vertex(const math::Vector3f& pos_ = math::Vector3f(), 
		const math::Vector3f& posParent_ = math::Vector3f()) 
		: pos(pos_), posParent(posParent_) {}

	math::Vector3f pos;
	math::Vector3f posParent;
	math::Vector2f parentTextureUV;
	math::Vector2f parentNormalUV;
};	

typedef unsigned short tri_index_type;

/*
chunk lod works as follows:

chunk is created
chunk is updated:

d = distance from camera to closest pt on bb
v = calculate_deviation(d)
if d > child_trigger_val
{
	if(!children)
		create children
	update children
	//set morph value on self to 1 - (d - child_trigger_val) / (trigger_val - child_trigger_val)
}
else if(children)
{
	delete children
}

if(root)
{
	d = distance from camera to closest pt on bb
	v = calculate_deviation(d)
	if(no geometry)
		create geometry
}


*/

struct ChunkLOD : public BatchableLand, public BatchableWater
{
	static const std::string CREATE_GEOMETRY_PROFILE;

	struct SharedData
	{
		int gridSize, textureRes;
		scene::transform::Group::ptr rootTransform;
		HeightDataProvider::ptr heightData;
		PlanetTextureBlender::ptr textureData;
		//render::SceneContext::ptr sceneContext;
		render::GeometrySet::ptr geometry;

		float radius, waterRadius;
		FramebufferObject::ptr texturingFBO;
		float maxResolution;

		scene::Material::ptr textureCreationMaterial;
		glbase::TriangleSet::ptr textureCreationTris;
		glbase::VertexSet::ptr textureCreationVerts;
	};

	struct State { enum type {
			Empty = 0,
			Prepared,
			//Built,
			Shown,
			PrepareChildren,
			ChildrenShown,
			StatesCount
	};};

	typedef std::shared_ptr < ChunkLOD > ptr;

	static void static_init();
	static void static_release();

	// create as root
	ChunkLOD(SharedData* shared,
		scene::transform::Group::ptr rootTransform,
		const std::vector<math::Vector3f>& sourcePts,
		glbase::Texture::ptr parentNormalTexture,
		glbase::Texture::ptr parentColourTexture,
		const math::Rectanglef& parentUVWindow);

	// create from parent
	ChunkLOD(const math::Rectanglei& parentArea, 
		const math::Rectanglef& parentUVWindow, 
		ChunkLOD* parent);

	~ChunkLOD();

	void update_parent_textures(glbase::Texture::ptr parentNormalTexture,
		glbase::Texture::ptr parentColourTexture,
		const math::Rectanglef& newParentUVWindow);

	struct Errors
	{
		Errors(float create_ = 10000.0f, float show_ = 10000.0f) : create(create_), show(show_) {}
		float create;
		float show;
	};

	void update(const math::Vector3f& cameraPosLocal, const math::Vector3f& predictedCameraPos, Errors errors = Errors());

	float calculate_distance_2( const math::Vector3f& localCameraPos ) const;

	const scene::transform::Group::ptr& get_root_transform() const { return _rootTransform; }

	bool is_shown_or_children_shown() const;

	size_t get_curr_max_depth() const;

	State::type get_state() const { return _state.get_state(); }

	template < class Fn >
	void apply_visitor(Fn fn);

	float get_blending_weight() const { return _currentBlendWeight; }

	static void delete_aborted_chunks();
	static void delete_chunk(ChunkLOD::ptr chunk);

	virtual void load_land_verts(BatchableLandVertex* targetVerts, float UVz);
	virtual glbase::TriangleSet::value_type load_land_indices(const glbase::TriangleSet::ptr& targetTris, glbase::TriangleSet::value_type offset, bool stitchStart);
	virtual glbase::Texture::ptr get_colour_texture() const;
	virtual glbase::Texture::ptr get_normal_texture() const;
	virtual void load_water_verts(BatchableWaterVertex* targetVerts);
	virtual glbase::TriangleSet::value_type load_water_indices(const glbase::TriangleSet::ptr& targetTris, glbase::TriangleSet::value_type offset, bool stitchStart);
private:
	ChunkLOD(const ChunkLOD& other);
	void operator=(const ChunkLOD& other);

	void init_state();
	bool set_state(State::type state);

	size_t get_vert_idx(size_t x, size_t y) const;

	void get_curr_max_depth_int(size_t& depth, size_t& maxDepth) const;
	void hide_all();

	void check_built();

	void update_children(const math::Vector3f& localCameraPos, const math::Vector3f& predictedCameraPos, const Errors& childErrors);
	bool show_children();
	bool hide_children();
	void check_children_built();

	bool create_geometry();			// Empty -> Prepared
	bool show();					// Prepared -> Shown
	bool create_children();			// Shown -> PrepareChildren
	bool show_children_and_hide();	// PrepareChildren -> ChildrenShown

	bool hide_children_and_show();	// ChildrenShown -> PrepareChildren
	bool delete_children();			// PrepareChildren -> Shown
	bool hide();					// Shown -> Prepared
	bool delete_geometry();			// Prepared -> Empty

	void delete_geometry_data();
	void clear();

	void abort_build();
	void wait_for_build();
	void load_geometry_from_thread_data();

	void update_UVs();

	void create_water_verts_not_root();
	void create_water_verts_root();
	void create_verts();


	void create_colour_texture();

	bool are_children_created() const;
	bool are_children_shown() const;
	bool are_children_built() const;

	bool is_shown() const;

	bool is_root() const;

	bool is_built() const;
	bool is_building() const;

	bool is_near() const; 

	//float get_child_create_error() const { return _childCreateError; }
	//float get_child_show_error() const { return _childShowError; }

	const math::AABBf& get_local_bounds() const;
	void update_parent_bounds();

	Errors calculate_errors(const math::Vector3f& localCameraPos, Errors& errors) const;


	//void create_skirt_edge(scene::VertexSet::ptr verts, scene::TriangleSet::ptr skirtTris, 
	//	int start, int end, const math::Vector2i& idxMask, 
	//	const math::Vector2i& offset, int newVertIdxOffset);

private:
	struct Quadrants { enum type {
		TOP_LEFT		= 0,
		TOP_RIGHT		= 1,
		BOTTOM_RIGHT	= 2,
		BOTTOM_LEFT		= 3
	};};

	SharedData* _shared;
	ChunkLOD::ptr _children[4];
	ChunkLOD* _parent;
	int _level;
	float _error, _childError;

	struct BuildingState { enum type {
		NotBuilding,
		Building,
		CreatingGeometry,
		GeneratingMips,
		Built
	};};

	BuildingState::type _buildingState;
	math::Rectanglei _parentArea;
	scene::Geometry::ptr _mesh, _waterMesh/*, _skirt*/;
	math::AABBf _localBounds; // this has to be guessed initially?
	glbase::Texture::ptr _normalTexture;
	glbase::Texture::ptr _colourTexture;
	glbase::Texture::ptr _parentNormalTexture;
	glbase::Texture::ptr _parentColourTexture;

	std::vector<math::Vector3f> _sourcePts;

	float _currentClosestDistance;
	//float _childShowError, _childCreateError;
	float _currentBlendWeight;
	bool _stateInit;
	float _landW, _waterW;
	misc::StateMachine<State> _state;
	scene::transform::Group::ptr _rootTransform;
	ChunkLODThreadData::ptr _buildThreadData;

	math::Rectanglef _parentNormalUVWindow;
	math::Rectangled _textureUVWindow;
	math::Rectanglef _parentTextureUVWindow;

	size_t _oneWeightFrames;
	BatchLandCache::ptr _chunkLandCache;
	BatchWaterCache::ptr _chunkWaterCache;
	bool _batched;

	static effect::Effect::ptr _shader;
	static effect::Effect::ptr _waterShader;
	static effect::Effect::ptr _waterL0Shader;

	static glbase::VertexSpec::ptr _vertexSpec;
	static glbase::VertexSpec::ptr _vertexWaterSpec;
	static glbase::VertexSpec::ptr _vertexWaterL0Spec;
	static glbase::VertexSpec::ptr _batchLandVertexSpec;
	static glbase::VertexSpec::ptr _batchWaterVertexSpec;


	//utils::Profiler::block_handle _createChunkProfileBlock;

#if defined(_DEBUG)
	std::deque<State::type> _DEBUG_stateChanges;
#endif
};

inline ChunkLOD::State::type& operator--(ChunkLOD::State::type& val)
{
	return (val = static_cast<ChunkLOD::State::type>(val - 1));
}

inline ChunkLOD::State::type& operator++(ChunkLOD::State::type& val)
{
	return (val = static_cast<ChunkLOD::State::type>(val + 1));
}

template < class Fn >
void ChunkLOD::apply_visitor(Fn fn)
{
	fn(this);
	if(are_children_created())
	{
		for(size_t idx = 0; idx < 4; ++idx)
		{
			_children[idx]->apply_visitor(fn);
		}
	}
}

}


#endif // __EXPLORE2_CHUNKLOD_H__