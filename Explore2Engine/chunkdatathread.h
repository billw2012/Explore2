#if !defined(__EXPLORE2_CHUNK_DATA_THREAD_H__)
#define __EXPLORE2_CHUNK_DATA_THREAD_H__

#include <iostream>

#include "Math/vector3.hpp"
#include "Math/vector4.hpp"
#include "Math/matrix4.hpp"

#include "heightdataprovider.h"
#include "textureprovider.h"

#include "databuilderthread.h"

#include "Scene/transform.hpp"

namespace explore2
{;

template < class FTy_ >
inline math::Vector3<FTy_> matrix_transform(const math::Matrix4<FTy_>& transform, const math::Vector3<FTy_>& vert)
{
	return math::Vector3<FTy_>(transform * (math::Vector4<FTy_>(vert) + math::Vector4<FTy_>::WAxis));
}


/*
chunk data provider:
When chunk data needs to be created, the required data to create it is
copied to a ChunkLODThreadData object, which is also where the data that
is being created is stored to.

The ChunkDataThread class is given this data object, and it adds it to the end
of its queue.
It runs a thread that spins waiting for an item in the queue
When there is an item:
feed it to a worker thread (which ever one has least left to do)
in the worker thread process all items in a queue then quit or spin waiting? 
creating threads is slow? spinning is slow?
generate the data for it.

in chunk code during update do not allow children to exist until parent data 
has been generated!
*/

struct ChunkLODThreadData : public BuildThreadData
{
	typedef std::shared_ptr< ChunkLODThreadData > ptr;

	// create for parent data
	ChunkLODThreadData(HeightDataProvider::ptr heightData,
		PlanetTextureBlender::ptr textureData);

	void init_parent(const math::Matrix4d& rootTransform,
		const std::vector<math::Vector3f>& sourcePts,
		int gridSize, int level, float radius, 
		bool generateWater = false, float waterRadius = 0.0f);

	void init_child(const math::Matrix4d& rootTransform,
		int gridSize, int level, float radius, 
		bool generateWater = false, float waterRadius = 0.0f);

	const math::Matrix4d&	get_root_transform		()				const	{ return _rootTransform; }
	const math::Vector3f&	get_parent_pos			(int x, int y)	const	{ return _parentPosData[get_parent_idx(x, y)]; }
	const math::Vector3f&	get_parent_parent_pos	(int x, int y)	const	{ return _parentParentPosData[get_parent_idx(x, y)]; }
	const math::Vector3f&	get_child_pos			(int x, int y)	const	{ return _newChildPosData[get_child_idx(x, y)]; }
	const math::Vector3f&	get_child_pos			(int idx)		const	{ return _newChildPosData[idx]; }
	const math::Vector3f&	get_child_water_pos		(int x, int y)	const	{ return _newChildWaterData[get_child_idx(x, y)]; }
	const math::Vector3f&	get_child_water_pos		(int idx)		const	{ return _newChildWaterData[idx]; }
	const math::Vector3f&	get_child_parent_pos	(int x, int y)	const	{ return _newChildParentPosData[get_child_idx(x, y)]; }
	const unsigned char*	get_normal				(int x, int y)	const	{ return &(_normalData[get_child_normal_idx(x, y)]); }
	unsigned char*			get_normal				(int x, int y)			{ return &(_normalData[get_child_normal_idx(x, y)]); }
	float					get_texture_weight		(int x, int y, int tex) const { return _textureWeights[tex][get_texture_weight_idx(x, y)]; }

	// normal and texture weight data conforms to this size
	int		get_doubled_grid_size() const { return _doubledGridSize; }
	// pos data conforms to this size
	int		get_extended_grid_size() const { return _extendedGridSize; }
	int		get_grid_size	()	const	{ return _gridSize; }
	int		get_level		()	const	{ return _level; }
	float	get_radius		()	const	{ return _radius; }

	bool	has_water		()	const	{ return _hasWater; }

	HeightDataProvider::ptr get_height_data() const { return _heightData; }

	void set_parent_pos			(int x, int y, const math::Vector3f& pos);
	void set_parent_parent_pos	(int x, int y, const math::Vector3f& pos);
	void set_child_pos			(int x, int y, const math::Vector3f& pos);
	void set_child_pos			(size_t idx, const math::Vector3f& pos);
	void set_child_water_pos	(int x, int y, const math::Vector3f& pos);
	void set_child_water_pos	(size_t idx, const math::Vector3f& pos);
	void set_normal				(int x, int y, const math::Vector3f& val);
	void set_child_parent_pos	(int x, int y, const math::Vector3f& pos);

	void copy_parent_pos		(const ChunkLODThreadData::ptr& from);

	void reset();

	int get_actual_grid_size() const { return (_gridSize * 2 + 2); }

	bool has_parent_data() const { return _parentPosData.size() != 0; }

	size_t get_parent_idx			(int x, int y) const;
	size_t get_child_idx			(int x, int y) const;
	size_t get_child_normal_idx		(int x, int y) const;
	size_t get_source_pt_idx		(int x, int y) const;
	size_t get_texture_weight_idx	(int x, int y) const;

	virtual void	process				(size_t step);
	virtual size_t	get_total_work		() const;
	virtual size_t	get_work_remaining	() const;

	ChunkLODThreadData(const ChunkLODThreadData& other);
	ChunkLODThreadData& operator=(const ChunkLODThreadData& other);

private: // funcs
	template < class FTy_ >	void generate_vert_data_without_parent	();
	template < class FTy_ >	void generate_vert_data_using_parent	();



private: // data
	math::Matrix4d _rootTransform;
	std::vector< math::Vector3f > _parentPosData; // should have a border of 1 
	std::vector< math::Vector3f > _parentParentPosData; // should have a border of 1 

	std::vector< math::Vector3f > _sourcePts; // if we don't have parent data we use this

	bool _generateWater, _hasWater;
	int _gridSize, _level, _extendedGridSize, _doubledGridSize;
	float _radius, _waterRadius;

	std::vector< math::Vector3f > _newChildPosData;	// should be double resolution with a border of 1
	std::vector< math::Vector3f > _newChildWaterData;	// should be double resolution with a border of 1
	std::vector< math::Vector3f > _newChildParentPosData; // should be double resolution with a border of 1
	std::vector< unsigned char > _normalData;
	std::vector< std::vector< float > > _textureWeights;

	HeightDataProvider::ptr _heightData;
	PlanetTextureBlender::ptr _textureData;

	size_t _totalWorkRemaining, _totalWork;
};

}
#endif