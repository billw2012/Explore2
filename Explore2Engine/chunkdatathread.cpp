#include "chunkdatathread.h"
//#define BOOST_BIND_ENABLE_STDCALL
#include "Utils/Profiler.h"

#undef min
#undef max

namespace explore2 {;

void ChunkLODThreadData::set_parent_pos(int x, int y, const math::Vector3f& pos)
{
	_parentPosData[get_parent_idx(x, y)] = pos;
}

void ChunkLODThreadData::set_parent_parent_pos(int x, int y, const math::Vector3f& pos)
{
	_parentParentPosData[get_parent_idx(x, y)] = pos;
}

void ChunkLODThreadData::set_child_pos(int x, int y, const math::Vector3f& pos)
{
	_newChildPosData[get_child_idx(x, y)] = pos;
}

void ChunkLODThreadData::set_child_pos(size_t idx, const math::Vector3f& pos)
{
	_newChildPosData[idx] = pos;
}

void ChunkLODThreadData::set_child_water_pos(int x, int y, const math::Vector3f& pos)
{
	_newChildWaterData[get_child_idx(x, y)] = pos;
}

void ChunkLODThreadData::set_child_water_pos(size_t idx, const math::Vector3f& pos)
{
	_newChildWaterData[idx] = pos;
}

void ChunkLODThreadData::set_child_parent_pos(int x, int y, const math::Vector3f& pos)
{
	_newChildParentPosData[get_child_idx(x, y)] = pos;
}

void ChunkLODThreadData::set_normal(int x, int y, const math::Vector3f& pos)
{
	size_t ncIdx = get_child_normal_idx(x, y);
	_normalData[ncIdx+0] = static_cast<unsigned char>(pos.x * 128 + 127);
	_normalData[ncIdx+1] = static_cast<unsigned char>(pos.y * 128 + 127);
	_normalData[ncIdx+2] = static_cast<unsigned char>(pos.z * 128 + 127);
}

void ChunkLODThreadData::copy_parent_pos(const ChunkLODThreadData::ptr& from)
{
}

size_t ChunkLODThreadData::get_parent_idx(int x, int y) const
{
	int idx = (y + 1) * (_gridSize + 2) + x + 1;
	assert(idx >= 0 && idx < ((_gridSize + 2) * (_gridSize + 2)));
	return idx;
}

size_t ChunkLODThreadData::get_child_idx(int x, int y) const
{
	int idx = (y + 1) * _extendedGridSize + x + 1;
	assert(idx >= 0 && idx < (_extendedGridSize * _extendedGridSize));
	return idx;
}

size_t ChunkLODThreadData::get_child_normal_idx(int x, int y) const
{
	int idx = y * _doubledGridSize + x;
	assert(idx >= 0 && idx < (_doubledGridSize * _doubledGridSize));
	return idx * 3;
}

size_t ChunkLODThreadData::get_source_pt_idx(int x, int y) const
{
	return y * 2 + x;
}

size_t ChunkLODThreadData::get_texture_weight_idx(int x, int y) const
{
	return y * _doubledGridSize + x;
}


void ChunkLODThreadData::process(size_t step)
{
	assert(step == 0);

	using namespace math;

	if(has_parent_data())
	{
		generate_vert_data_using_parent<double>();
	}
	else
	{
		generate_vert_data_without_parent<double>();
	}

	// calculating final data
	math::SSETraits<float>::Vector heights, polarAngles, verticalAngles;
	for(int y = 0; y < _doubledGridSize; ++y)
	{
		for(int x = 0; x < _doubledGridSize; ++x)
		{
			const Vector3f& origin = get_child_pos(x, y);
			Vector3f down(get_child_pos(x, y+1) - origin);
			Vector3f up(get_child_pos(x, y-1) - origin);
			Vector3f right(get_child_pos(x+1, y) - origin);
			Vector3f left(get_child_pos(x-1, y) - origin);

			Vector3f norm1 = down.crossp(right).normal();
			Vector3f norm2 = right.crossp(up).normal();
			Vector3f norm3 = up.crossp(left).normal();
			Vector3f norm4 = left.crossp(down).normal();
			Vector3f norm = (norm1 + norm2 + norm3 + norm4).normal();
			set_normal(x, y, Vector3f(norm));

			Vector3d vec(matrix_transform(_rootTransform, Vector3d(origin)));
			heights.push_back(((float)vec.length() - _radius) / (float)_heightData->scale());
			polarAngles.push_back((float)vec.angle(Vector3d::YAxis));
			verticalAngles.push_back((float)norm.angle(vec.normal())); // this assumes no rotation in the root transform!!
		}
	}

	_textureData->get_texture_weights(heights, polarAngles, verticalAngles, _textureWeights);

	_totalWorkRemaining = 0;
}

size_t ChunkLODThreadData::get_total_work() const
{
	return _totalWork;
}

size_t ChunkLODThreadData::get_work_remaining() const
{
	return _totalWorkRemaining;
}

template < class FTy_ >	
void ChunkLODThreadData::generate_vert_data_without_parent()
{
	using namespace math;

	//misc::SIMDSoAVector3 verts, vertsHigh;
	HeightDataProvider::pts_vector_type verts;
	//std::vector<math::Vector3d> verts;
	
	// this loop can be decomposed into a set of loops for each condition, 
	// and have vars moved out of the inner loops to optimize it. 
	int doubleGridSize = _doubledGridSize;
	int idx = 0;
	float fGridSize = static_cast<float>(doubleGridSize-1);
	for(int y = -1; y < doubleGridSize + 1; ++y)
	{
		float ySourceQuadIdx = (y / fGridSize);
		int ySourceQuadIdxi = 0;
		float yRemainder = ySourceQuadIdx - ySourceQuadIdxi;
		float oneMinusyRemainder = 1 - yRemainder;
		for(int x = -1; x < doubleGridSize + 1; ++x, ++idx)
		{
			float xSourceQuadIdx = (x / fGridSize);
			int xSourceQuadIdxi = 0;
			float xRemainder = xSourceQuadIdx - xSourceQuadIdxi;

			math::Vector3f p0(_sourcePts[get_source_pt_idx(xSourceQuadIdxi, ySourceQuadIdxi)]);
			math::Vector3f p1(_sourcePts[get_source_pt_idx(xSourceQuadIdxi+1, ySourceQuadIdxi)]);
			math::Vector3f p2(_sourcePts[get_source_pt_idx(xSourceQuadIdxi+1, ySourceQuadIdxi+1)]);
			math::Vector3f p3(_sourcePts[get_source_pt_idx(xSourceQuadIdxi, ySourceQuadIdxi+1)]);

			math::Vector3f cpPt;
			if(x > 0 && x < doubleGridSize-2 && y > 0 && y < doubleGridSize-2)
			{
				float sumRem = xRemainder + yRemainder;
				if(sumRem <= 1.0)
				{
					math::Vector3f leftDUV = p3 - p0;
					math::Vector3f topDUV = p1 - p0;
					math::Vector3f epth = p0 + topDUV * sumRem;
					math::Vector3f eptv = p0 + leftDUV * sumRem;
					math::Vector3f dhv = eptv - epth;
					cpPt = epth + dhv * ((sumRem > 0.0)? (yRemainder / sumRem) : 1);
				}
				else
				{
					math::Vector3f rightDUV = p1 - p2;
					math::Vector3f bottomDUV = p3 - p2;
					float oneMinusxRemainder = 1 - xRemainder;
					sumRem = oneMinusxRemainder + oneMinusyRemainder;
					math::Vector3f epth = p2 + bottomDUV * sumRem;
					math::Vector3f eptv = p2 + rightDUV * sumRem;
					math::Vector3f dhv = eptv - epth;
					cpPt = epth + dhv * (oneMinusyRemainder / sumRem);
				}
			}
			else
			{
				math::Vector3f leftDUV = p3 - p0;
				math::Vector3f rightDUV = p2 - p1;
				math::Vector3f leftPt = p0 + leftDUV * yRemainder;
				math::Vector3f rightPt = p1 + rightDUV * yRemainder;
				math::Vector3f leftRight = rightPt - leftPt;
				cpPt = leftPt + leftRight * xRemainder;
			}
			set_child_parent_pos(x, y, cpPt);
			math::Vector3<FTy_> localPos(matrix_transform(math::Matrix4<FTy_>(get_root_transform()), 
				math::Vector3<FTy_>(get_child_parent_pos(x, y))));
			verts.push_back(HeightDataProvider::pts_vector_type::value_type(localPos.normal()));
			//vertsHigh.push_back(localPos);
		}
	}

	if(is_aborted())
		return;

	HeightDataProvider::results_vector_type heights = get_height_data()->get(verts, false);

	//std::vector<float> heights(verts.size());
	//static const size_t CalcMax = 2048;
	//size_t partRem = verts.size() % CalcMax;
	//size_t partCount = (verts.size() - partRem) / CalcMax;

	//for(size_t idx = 0; idx < partCount; ++idx)
	//{
	//	HeightDataProvider::results_vector_type partHeights = get_height_data()->get(&verts[idx * CalcMax], CalcMax, false);
	//	memcpy_s(&heights[idx * CalcMax], CalcMax * sizeof(float), &partHeights[0], CalcMax * sizeof(HeightDataProvider::results_vector_type::value_type));
	//	if(is_aborted())
	//		return;
	//}
	//if(partRem)
	//{
	//	HeightDataProvider::results_vector_type partHeights = get_height_data()->get(&verts[partCount * CalcMax], partRem, false);
	//	memcpy_s(&heights[partCount * CalcMax], partRem * sizeof(float), &partHeights[0], partRem * sizeof(HeightDataProvider::results_vector_type::value_type));
	//}
	

	if(is_aborted())
		return;

	Matrix4<FTy_> inverseRootTransform(get_root_transform().inverse());

	_hasWater = false;

	for(int y = -1, idx = 0; y < doubleGridSize + 1; ++y)
	{
		for(int x = -1; x < doubleGridSize + 1; ++x, ++idx)
		{
			FTy_ heightScalar = heights[idx] * (FTy_)get_height_data()->scale(); 
			Vector3<FTy_> localVec(matrix_transform(inverseRootTransform, 
				Vector3<FTy_>(verts[idx]) * (FTy_)get_radius()));
			Vector3f scaledVec(localVec + Vector3<FTy_>(verts[idx]) * heightScalar);
			set_child_pos(x, y, scaledVec);

			if(_generateWater)
			{
 				FTy_ totalHeight = heights[idx] * (FTy_)get_height_data()->scale() + _radius;
				_hasWater = _hasWater || (totalHeight < _waterRadius);
				math::Vector3f waterVert(matrix_transform(inverseRootTransform, Vector3<FTy_>(verts[idx]) * (FTy_)_waterRadius));
				set_child_water_pos(x, y, waterVert);
			}
		}
	}
}

template < class FTy_ >
void ChunkLODThreadData::generate_vert_data_using_parent()
{
	// generate parent pos data

	using namespace math;

	int doubleGridSize = _doubledGridSize;

	std::vector<size_t> vertIndicies;
	HeightDataProvider::pts_vector_type verts;

	math::Matrix4d inverseRootTransform = get_root_transform().inverse();

	_hasWater = false;

	bool eveny = false;
	for(int y = -1; y < doubleGridSize+1; ++y, eveny = !eveny)
	{
		bool evenx = false; 
		for(int x = -1; x < doubleGridSize+1; ++x, evenx = !evenx) // saved from doing the mod
		{
			Vector3f pVert, pParentVert;
			if(evenx && eveny)
			{
				// We actually need to re-calculate the parent positions if they
				// are odd with respect to the parent grid. This is because we are 
				// creating a grid at twice the resolution, so only every 2nd
				// vertex is going to get displayed. This translates to every 
				// fourth vertex matching the parent, and every 2nd vertex needing 
				// to be set to the parents averaged position.
				bool pevenx = (x % 4) == 0;
				bool peveny = (y % 4) == 0;
				if(pevenx && peveny)
					pVert = get_parent_pos(x/2, y/2);
 				else if(pevenx && !peveny)
 				{
 					pVert = (get_parent_pos(x/2, (y-2)/2) + 
 						get_parent_pos(x/2, (y+2)/2)) * 0.5f;
 				}
 				else if(!pevenx && peveny)
 				{
 					pVert = (get_parent_pos((x-2)/2, y/2) + 
 						get_parent_pos((x+2)/2, y/2)) * 0.5f;
 				}
 				else if(!pevenx && !peveny)
 				{
 					pVert = (get_parent_pos((x-2)/2, (y+2)/2) + 
 						get_parent_pos((x+2)/2, (y-2)/2)) * 0.5f;
 				}
				set_child_pos(x, y, get_parent_pos(x/2, y/2));

				if(_generateWater)
				{
					math::Vector3<FTy_> globalpVert(matrix_transform(get_root_transform(),
						math::Vector3<FTy_>(pVert)));
					math::Vector3f waterVert(matrix_transform(inverseRootTransform, 
						globalpVert.normal() * (FTy_)_waterRadius));
					set_child_water_pos(x, y, waterVert);
				}
			}
			else
			{
				if(evenx && !eveny)
				{
					pVert = (get_parent_pos(x/2, (y-1)/2) + 
						get_parent_pos(x/2, (y+1)/2)) * 0.5f;
				}
				else if(!evenx && eveny)
				{
					pVert = (get_parent_pos((x-1)/2, y/2) + 
						get_parent_pos((x+1)/2, y/2)) * 0.5f;
				}
				else if(!evenx && !eveny)
				{
					pVert = (get_parent_pos((x-1)/2, (y+1)/2) + 
						get_parent_pos((x+1)/2, (y-1)/2)) * 0.5f;
				}
				vertIndicies.push_back(get_child_idx(x, y));
				math::Vector3<FTy_> globalpVert(matrix_transform(
					get_root_transform(), math::Vector3<FTy_>(pVert)));
				verts.push_back(HeightDataProvider::pts_vector_type::value_type(globalpVert.normal()));
				if(_generateWater)
				{
					math::Vector3f waterVert(matrix_transform(inverseRootTransform, 
						globalpVert.normal() * (FTy_)_waterRadius));
					_hasWater = _hasWater || (globalpVert.length() < _waterRadius);
					set_child_water_pos(x, y, waterVert);
				}
			}
			set_child_parent_pos(x, y, pVert);
		}
	}

	if(is_aborted())
		return;

	HeightDataProvider::results_vector_type heights = get_height_data()->get(verts, false);

	if(is_aborted())
		return;

	for(size_t idx = 0; idx < vertIndicies.size(); ++idx)
	{
		// more accurate to do it like this than to normalise the vert first 
		// then multiply by the height and add two vecs ?? doesn't require a sqr root :)
		FTy_ heightScalar = heights[idx] * (FTy_)get_height_data()->scale(); 
		Vector3<FTy_> localVec(matrix_transform(inverseRootTransform, Vector3<FTy_>(verts[idx]) * (FTy_)get_radius()));
		Vector3f scaledVec(localVec + Vector3<FTy_>(verts[idx]) * heightScalar);
		set_child_pos(vertIndicies[idx], scaledVec);
		if(_generateWater)
		{
			FTy_ totalHeight = heightScalar + (FTy_)get_radius();
			_hasWater = _hasWater || (totalHeight < _waterRadius);
		}
	}
}

void ChunkLODThreadData::reset()
{
	_newChildPosData.swap(std::vector<math::Vector3f>());
	_newChildParentPosData.swap(std::vector<math::Vector3f>());
	_newChildWaterData.swap(std::vector<math::Vector3f>());
	_parentPosData.swap(std::vector<math::Vector3f>());
	reset_flags();
}

ChunkLODThreadData::ChunkLODThreadData(HeightDataProvider::ptr heightData,
									   PlanetTextureBlender::ptr textureData ) 
									   : _heightData(heightData),
									   _textureData(textureData)
{

}

ChunkLODThreadData::ChunkLODThreadData( const ChunkLODThreadData& other ) : _rootTransform(other._rootTransform), 
	_parentPosData(other._parentPosData),
	_sourcePts(other._sourcePts),
	_gridSize(other._gridSize), _level(other._level), _radius(other._radius),
	_newChildPosData(other._newChildPosData), 
	_newChildParentPosData(other._newChildParentPosData), 
	_newChildWaterData(other._newChildWaterData),
	_generateWater(other._generateWater), _waterRadius(other._waterRadius),
	_heightData(other._heightData)
{

}

ChunkLODThreadData& ChunkLODThreadData::operator=( const ChunkLODThreadData& other )
{
	_rootTransform = other._rootTransform; 
	_parentPosData = other._parentPosData;
	_sourcePts = other._sourcePts;
	_gridSize = other._gridSize; _level = other._level; _radius = other._radius;
	_generateWater = other._generateWater; _waterRadius = other._waterRadius;
	_newChildPosData = other._newChildPosData; 
	_newChildParentPosData = other._newChildParentPosData; 
	_newChildWaterData = other._newChildWaterData; 
	_heightData = other._heightData;
	reset_flags();
	if(other.is_aborted())
		set_abort_build();
	if(other.is_finished_build())
		set_finished_build();
	_totalWork = 1;
	_totalWorkRemaining = 1;

	return *this;
}

void ChunkLODThreadData::init_parent(const math::Matrix4d& rootTransform, 
									 const std::vector<math::Vector3f>& sourcePts,
									 int gridSize, int level, float radius, 
									 bool generateWater, float waterRadius)
{
	_rootTransform = rootTransform;
	_sourcePts = sourcePts;
	_gridSize = gridSize; _level = level; _radius = radius; 
	_generateWater = generateWater; _waterRadius = waterRadius;
	_parentPosData.resize(0);
	_parentParentPosData.resize(0);
	// see init_child for explanation of this
	_extendedGridSize = (gridSize - 1) * 2 + 1 + 2;
	_newChildPosData.resize(_extendedGridSize * _extendedGridSize);
	_newChildParentPosData.resize(_extendedGridSize * _extendedGridSize);
	_newChildWaterData.resize(generateWater? _extendedGridSize * _extendedGridSize : 0);
	// see init_child for explanation of this
	_doubledGridSize = (gridSize - 1) * 2 + 1;
	_normalData.resize(_doubledGridSize * _doubledGridSize * 3);
	reset_flags();

	size_t workForOne = _extendedGridSize * _extendedGridSize;
	_totalWork = _totalWorkRemaining = generateWater ? workForOne * 2 : workForOne;
}

void ChunkLODThreadData::init_child(const math::Matrix4d& rootTransform, 
									int gridSize, int level, float radius, 
									bool generateWater, float waterRadius)
{
	_rootTransform = rootTransform;
	_gridSize = gridSize; _level = level; _radius = radius; 
	_generateWater = generateWater; _waterRadius = waterRadius;
	// base grid, then add intermediate points and edge points.
	// fence post problem, 4 quads means 5 edges, so baseGrid = 5 for 4 quads.
	// so modified grid with edges and extra intermediate points = (5 - 1) * 2 + 1.
	// 5 - 1 to subtract the last fence post, * 2 to add intermediate points, + 1 to add last fence post again.
	// then + 2 for the extended points
	_extendedGridSize = (gridSize - 1) * 2 + 1 + 2;
	// parent data is gridSize + 2 for the extra edge points
	_parentPosData.resize((gridSize + 2) * (gridSize + 2));
	_parentParentPosData.resize((gridSize + 2) * (gridSize + 2));
	_newChildPosData.resize(_extendedGridSize * _extendedGridSize);
	_newChildParentPosData.resize(_extendedGridSize * _extendedGridSize);
	_newChildWaterData.resize(generateWater? _extendedGridSize * _extendedGridSize : 0);
	// final normal data is gridSize + intermediate points, NO extended points so (gridSize - 1) * 2 + 1
	_doubledGridSize = (gridSize - 1) * 2 + 1;
	_normalData.resize(_doubledGridSize * _doubledGridSize * 3);
	reset_flags();
	size_t workForOne = _extendedGridSize * _extendedGridSize;
	_totalWork = _totalWorkRemaining = generateWater ? workForOne * 2 : workForOne;
}

}