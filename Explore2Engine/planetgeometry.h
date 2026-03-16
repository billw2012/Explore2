#if !defined(__EXPLORE2_PLANET_H__)
#define __EXPLORE2_PLANET_H__

#include "chunklod.h"
#include "Render/scenecontext.hpp"

#include "atmospherics.h"

#include <unordered_map>
#include <unordered_set>

#include <cstddef>

namespace explore2 { ;

/*
ItemType must conform to:
struct ItemType
{
	math::AABBf get_bounds() const;
};
*/

template < class ItemType >
struct SpatialPartition
{
	typedef ItemType item;
	typedef std::set<item*> ItemPtrSet;

	struct Node;
	typedef std::vector<Node> NodeVector;
	struct Node
	{
		ItemPtrSet items;
		NodeVector children;
		math::AABBf bounds;
	};

	SpatialPartition(size_t leafSize = 1) : _leafSize(leafSize) {}

	void create(const ItemPtrSet& items)
	{
		math::AABBf fullBounds;

		_root = Node();
		for(ItemPtrSet::const_iterator iItr = items.begin(); iItr != items.end(); ++iItr)
		{
			item* pItem = *iItr;
			_root.bounds.expand(pItem->get_bounds());
		}
		_root.items = items;
		if(_root.items.size() > _leafSize)
			split(_root);
	}

	void get_items_in_range(const math::Vector3f& pos, 
		float range, ItemPtrSet& items)
	{
		get_items_in_range(pos, range*range, items, _root);
	}

private:
	void get_items_in_range(const math::Vector3f& pos, 
		float rangeSquared, ItemPtrSet& items, Node& node)
	{
		if((node.bounds.closest_point(pos) - pos).lengthSquared() <= rangeSquared)
		{
			items.insert(node.items.begin(), node.items.end());
			for(size_t idx = 0; idx < node.children.size(); ++idx)
			{
				get_items_in_range(pos, rangeSquared, items, node.children[idx]);
			}
		}
	}

	void split(Node& parent)
	{
		math::Vector3f extents = parent.bounds.extents();
		int longestEdge = (extents.x > extents.y) ? 
			((extents.x > extents.z) ? 0 : 2) : ((extents.y > extents.z)? 1 : 2);

		float divPos = parent.bounds.min()[longestEdge] + extents[longestEdge] * 0.5f;

		parent.children.resize(2);
		math::AABBf& leftBounds = parent.children[0].bounds;
		math::AABBf& rightBounds = parent.children[1].bounds;
		ItemPtrSet& leftSet = parent.children[0].items;
		ItemPtrSet& rightSet = parent.children[1].items;

		for(ItemPtrSet::const_iterator iItr = parent.items.begin(); iItr != parent.items.end(); ++iItr)
		{
			item* pItem = *iItr;

			if(pItem->get_bounds().center()[longestEdge] <= divPos)
			{
				leftSet.insert(pItem);
				leftBounds.expand(pItem->get_bounds());
			}
			else
			{
				rightSet.insert(pItem);
				rightBounds.expand(pItem->get_bounds());
			}
		}
		parent.items.clear();

		if(leftSet.size() > _leafSize)
			split(parent.children[0]);
		if(rightSet.size() > _leafSize)
			split(parent.children[1]);
	}

private:
	Node _root;
	size_t _leafSize;
};

struct PlanetSurfaceVertex
{
	PlanetSurfaceVertex(const math::Vector3f& pos_, 
		const math::Vector2f& uv_) : pos(pos_), uv(uv_) {}
	PlanetSurfaceVertex(const math::Vector3f& pos_ = math::Vector3f()) 
		: pos(pos_) {}

	math::Vector3f pos;
	math::Vector2f uv;
};	

struct PlanetGeometry
{
	typedef std::shared_ptr<PlanetGeometry> ptr;

	struct DeferredCollision
	{
		friend struct PlanetGeometry;
		typedef std::shared_ptr<DeferredCollision> ptr;
		scene::transform::Transform::vec3_type get() const;
	private:
		DeferredCollision(double radius_, double minRange_, const HeightDataProvider::NoiseGen::NoiseExecutionPtr& exec_, const scene::transform::Transform::vec3_type& localpt_);
		double radius;
		double minRange;
		HeightDataProvider::NoiseGen::NoiseExecutionPtr exec;
		scene::transform::Transform::vec3_type localpt;
	};

	struct State { enum type {
		Empty = 0,		// all updates and geometry disabled 
		Prepared,		// should create the planet
		Shown,			// should show the planet
		ChunksActive,	// should show chunks
		StatesCount
	};};
	
	PlanetGeometry();

	static void static_init();
	static void static_release();

	void init(scene::transform::Group::ptr parent, 
		render::GeometrySet::ptr geometry,
		render::GeometrySet::ptr distantGeometry,
		AtmosphereParameters::ptr atmosphere,
		float radius, bool water, float waterRadius, 
		int gridSize = 17, 
		int initalTextureSize = 8, 
		int targetTextureSize = 1024, 
		int chunkTextureSize = 256,
		float heightScaleFactor = 0.01f,
		unsigned short seed = 0);

	void update(scene::transform::Camera::ptr camera, const scene::transform::Transform::vec3_type& cameraVelocitySeconds);

	size_t get_curr_max_chunk_depth() const;

	template < class Fn >
	void apply_visitor_to_chunks(Fn fn);

	PlanetGeometry::DeferredCollision::ptr collide(scene::transform::Transform::vec3_type pt, double minRange = 1.8) const;

	scene::transform::Group::ptr get_parent_transform() const;

	State::type get_state() const { return _state.get_state(); }
	bool is_visible() const { return _state.get_state() >= State::Shown; }

	AtmosphereParameters::ptr get_atmosphere() const { return _atmosphere; }

	render::GeometrySet::ptr get_geometry_set() const { return _sharedChunkData.geometry; }

	float get_max_height() const;

private:
	PlanetGeometry(const PlanetGeometry&);
	void operator=(const PlanetGeometry&);

	void create_distant_geom(const render::GeometrySet::ptr& distantGeometry, const scene::transform::Group::ptr& parent);

	void update_chunks(const  scene::transform::Camera::ptr& camera, const scene::transform::Transform::vec3_type& cameraVelocitySeconds);

	bool set_state(State::type newState);
	// state transition functions
	bool prepare();	// Empty -> Prepared
	bool show();	// Prepared -> Shown
	bool create_chunks(); // Shown -> ChunksActive
	bool delete_chunks(); // ChunksActive -> Shown
	bool hide();	// Shown -> Prepared
	bool destroy();	// Prepared -> Empty

	void abort_build();
	void check_built();

	void init_state();

	bool is_built();
	bool is_building();

	void check_texture_update();
	//void check_atmosphere_update();

	void abort_chunks_build();
	void check_chunks_built();

	void load_geometry_from_thread_data();
	//void create_chunk_partitions();
	//void create_and_activate_chunk_lods();

	void load_textures_from_thread_data();
	//void load_atmosphere_from_thread_data();

	void init_adjacency();

	struct ChunkData
	{
		ChunkData(const ChunkLOD::ptr& chunk_ = ChunkLOD::ptr(), 
			const math::Vector2i& loc_ = math::Vector2i())
			: chunk(chunk_), loc(loc_) {}

		ChunkLOD::ptr chunk;
		math::Vector2i loc;
	};
	struct ChunkPartitionItem
	{
		ChunkPartitionItem() {}

		const math::AABBf& get_bounds() const
		{
			return bounds;
		}

		math::AABBf bounds;
		size_t faceIdx;
		math::Vector2i faceCoords;
		std::vector<size_t> verts;
	};
	typedef std::unordered_map< ChunkPartitionItem*, ChunkData > ChunkPartitionMap;

	bool update_chunk(ChunkData& chunkData, const scene::transform::Camera::ptr& camera, scene::transform::Transform::vec3_type predictedCameraPos, std::unordered_set< math::Vector2i >& shownChunks);
	void activate_chunk(ChunkPartitionItem* chunkData);
	void deactivate_chunk(ChunkPartitionItem* chunkData);


	struct FaceData
	{
		scene::Geometry::ptr mesh;
		glbase::Texture::ptr normalTexture, colourTexture, heightTexture;
		size_t ptIdxOffset;
	};

	static void createSphere(size_t gridSize, float radius, bool water, float waterRadius,
		const scene::transform::Transform::matrix4_type& rootTransform,
		HeightDataProvider& heightData,
		std::vector<PlanetSurfaceVertex>& pts);

	static void update_UVs(size_t gridSize, size_t ptIdxOffset,
		PlanetSurfaceVertex* pts, size_t textureSize);

	static void createFace(const std::vector< math::Vector3f >& left, bool revL, 
		const std::vector< math::Vector3f >& right, bool revR,
		HeightDataProvider::pts_vector_type& normalPts, 
		std::vector< PlanetSurfaceVertex >& pts);

	static tri_index_type buildTristrip(size_t gridSize, size_t ptIdxOffset, 
		const std::unordered_set< math::Vector2i >& gaps, std::vector< tri_index_type >& tristrip);

	static void buildSkirts(size_t gridSize, size_t ptIdxOffset, tri_index_type lastpt, const std::unordered_set< math::Vector2i >& gaps, std::vector< tri_index_type >& triStrip);
	static void createEdge(int ptCount, const math::Vector3f& startpt, const math::Vector3f& endpt, 
		std::vector< math::Vector3f >& verts, int border = 0);

	static void createTextureSide(std::vector<unsigned char>& bits, 
		HeightDataProvider& heightData,
		const PlanetTextureBlender& textureData,
		std::vector<math::Vector3f>& heightbits, 
		std::vector<unsigned char>& normalbits, 
		std::vector<float>& heights, 
		const math::Vector3f& orientation,
		float radius,
		size_t textureSize);

	struct PlanetGeometryBuildData : BuildThreadData
	{
		typedef std::shared_ptr<PlanetGeometryBuildData> ptr;

		PlanetGeometryBuildData() {}
		PlanetGeometryBuildData(HeightDataProvider::ptr heightData);

		void init(size_t gridSize, float radius, bool water, float waterRadius, const scene::transform::Transform::matrix4_type& rootTransform);

		virtual void process(size_t step);
		virtual size_t get_total_work() const;
		virtual size_t get_work_remaining() const;

		const PlanetSurfaceVertex& get_pt(size_t idx) const { return _pts[idx]; }
		const PlanetSurfaceVertex& get_pt(size_t x, size_t y, size_t ptIdx, size_t face) const { return _pts[get_pt_idx(x, y, ptIdx, face)]; }
		const PlanetSurfaceVertex* get_data() const { return &(_pts[0]); }
		PlanetSurfaceVertex* get_data() { return &(_pts[0]); }

		size_t get_pts_count() const { return _pts.size(); }
		size_t get_pt_idx(size_t x, size_t y, size_t ptIdx, size_t face) const { return (face * _gridSize * _gridSize + y * _gridSize + x) * 2 + ptIdx; }
	private:
		size_t _workRemaining, _totalWork;
		size_t _gridSize;
		float _radius;
		bool _water;
		float _waterRadius;
		scene::transform::Transform::matrix4_type _rootTransform;
		HeightDataProvider::ptr _heightData;
		std::vector<PlanetSurfaceVertex> _pts;
	};

	struct PlanetTextureBuildData : BuildThreadData
	{
		typedef std::shared_ptr<PlanetTextureBuildData> ptr;

		PlanetTextureBuildData() {}
		PlanetTextureBuildData(HeightDataProvider::ptr heightData,
			PlanetTextureBlender::ptr textureData);

		void init(size_t textureSize, float radius);

		virtual void process(size_t step);
		virtual size_t get_total_work() const;
		virtual size_t get_work_remaining() const;

		size_t get_texture_size() const { return _textureSize; }

		unsigned char* get_normal_texture(size_t idx) { return &(_normals[idx][0]); }
		unsigned char* get_colour_texture(size_t idx) { return &(_colours[idx][0]); }
		float* get_height_texture(size_t idx) { return &(_heights[idx][0]); }

	private:
		size_t _workRemaining, _totalWork;
		HeightDataProvider::ptr _heightData;
		PlanetTextureBlender::ptr _textureData;
		std::vector< std::vector< float > > _heights;
		std::vector< std::vector< unsigned char > > _normals, _colours;
		size_t _textureSize;
		float _radius;
	};

	typedef SpatialPartition<ChunkPartitionItem> ChunkSpatialPartition;

	struct PlanetChunksBuildData : BuildThreadData
	{
		typedef std::shared_ptr<PlanetChunksBuildData> ptr;

		PlanetChunksBuildData() : _geometry(NULL) {}

		void init(size_t gridSize, const PlanetGeometryBuildData::ptr& geometry);

		virtual void process(size_t step);
		virtual size_t get_total_work() const;
		virtual size_t get_work_remaining() const;

		ChunkSpatialPartition& get_chunk_partitions() { return _chunkPartitions; }
		const ChunkSpatialPartition& get_chunk_partitions() const  { return _chunkPartitions; }

	private:
		size_t _gridSize;
		PlanetGeometryBuildData::ptr _geometry;
		std::vector< ChunkPartitionItem > _chunkPartitionItems;
		ChunkSpatialPartition _chunkPartitions;
		size_t _workRemaining, _totalWork;
	};


	struct AdjacencyInfo
	{
		struct Side
		{
			Side() : faceIndex(0xffffFFFF) {}

			Side(size_t faceIndex_, const math::Matrix3i& rot_)
				: faceIndex(faceIndex_), rot(rot_) {}

			size_t faceIndex;
			math::Matrix3i rot;
		};

		Side top, left, right, bottom;
	};

private:
	ChunkLOD::SharedData _sharedChunkData;

	bool _water;
	AtmosphereParameters::ptr _atmosphere;

	size_t _gridSize;
	size_t _initalTextureSize, _currentTextureSize, _targetTextureSize;
	float _pixelAreaToCreate;
	float _pixelAreaToShow;
	float _pixelAreaToActivateChunks;
	float _chunkActivationDistance;
	bool _built, _building, _buildingTexture;
	bool _chunksBuilt, _creatingChunks;

	float _heightScaleFactor;

	glbase::VertexSpec::ptr _vertexSpec;
	FaceData _faces[6];

	PlanetGeometryBuildData::ptr _geometryThreadData;
	PlanetTextureBuildData::ptr _textureThreadData;
	PlanetChunksBuildData::ptr _chunkThreadData;

	PlanetTexture::ptr _waterTexture;

	misc::StateMachine<State> _state;

	ChunkPartitionMap _activeChunks;

	AdjacencyInfo _adjaceny[6];

	static effect::Effect::ptr _surfaceShader;
	static effect::Effect::ptr _textureCreationShader;
	static effect::Effect::ptr _planetBillboardShader;
	static glbase::VertexSpec::ptr _planetBillboardVertexSpec;
	static glbase::Texture::ptr _planetBillboardTexture;
};

inline PlanetGeometry::State::type& operator--(PlanetGeometry::State::type& val)
{
	return (val = static_cast<PlanetGeometry::State::type>(val - 1));
}

inline PlanetGeometry::State::type& operator++(PlanetGeometry::State::type& val)
{
	return (val = static_cast<PlanetGeometry::State::type>(val + 1));
}

template < class Fn >
void PlanetGeometry::apply_visitor_to_chunks(Fn fn)
{
	for(ChunkPartitionMap::iterator cItr = _activeChunks.begin(); 
		cItr != _activeChunks.end(); ++cItr)
	{
		cItr->second.chunk->apply_visitor(fn);
	}

	//for(size_t idx = 0; idx < 6; ++idx)
	//{
	//	for(size_t cIdx = 0; cIdx < _faces[idx].chunkData.size(); ++cIdx)
	//	{
	//		if(_faces[idx].chunkData[cIdx].chunk)
	//			_faces[idx].chunkData[cIdx].chunk->apply_visitor(fn);
	//	}
	//}
}

}
#endif // __EXPLORE2_PLANET_H__