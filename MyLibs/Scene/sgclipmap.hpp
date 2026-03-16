#ifndef _SCENE_SGCLIPMAP_HPP
#define _SCENE_SGCLIPMAP_HPP

#include <deque>
#include <sstream>

#include "Math/vector2.hpp"
#include "Math/vector3.hpp"
#include "Math/vector4.hpp"
#include "Math/matrix4.hpp"
#include "Math/ray.hpp"
#include "Math/plane.hpp"
#include "Math/rectangle.hpp"
#include "Math/intersection.hpp"
#include "Math/perlin.hpp"
#include "group.hpp"
#include "vertexspec.hpp"
#include "vertexset.hpp"
#include "GLBase/texture.hpp"
#include "fbo/framebufferObject.h"
#include "camera.hpp"

#define MY_MIN(a, b)	__min(a, b)
#define MY_MAX(a, b)	__max(a, b)

namespace scene
{

// base class from which to derive height data providers
template < class FloatType >
class DefaultHeightDataProvider
{
public:
	typedef FloatType value_type;
	typedef DefaultHeightDataProvider < value_type > this_type;
	typedef std::shared_ptr< this_type > ptr;

private:
	static const float PerlinPersistence;
	static const unsigned int PerlinDepth;
	math::Perlin< float > _height;

public:
	DefaultHeightDataProvider() : _height(PerlinDepth) {}

	virtual value_type get(const math::Vector3<value_type>& vec)
	{
		return _height.perlin3D(vec, PerlinPersistence, PerlinDepth);
	}
};

template < class F > const float DefaultHeightDataProvider<F>::PerlinPersistence = 0.58f;
template < class F > const unsigned int DefaultHeightDataProvider<F>::PerlinDepth = 20;

// regular nested grids representing geometry of a landscape.
// wrapped around a sphere of specified diameter

// default traits for sgcs
template < 
	class CoordFloatType, 
	class TriangleIndexType,
	class MatrixFloatType = CoordFloatType, 
	class _HeightDataProviderType = DefaultHeightDataProvider< CoordFloatType > 
>
struct DefaultSGCTraits
{
	typedef CoordFloatType coord_float_type;
	typedef MatrixFloatType matrix_float_type;
	typedef _HeightDataProviderType HeightDataProviderType;
	typedef TriangleIndexType triangle_index_type;
	typedef transform::Group< matrix_float_type > GroupType;
	typedef transform::DefaultMatrixTraits<matrix_float_type> MatrixTraits;
};

// encapsulates a level of the geometry clip map
template < 
	class FloatType,
	class TriangleIndexType, 
	class SGCTraits = DefaultSGCTraits< FloatType, TriangleIndexType >
>
struct SGCLevel
{
	typedef unsigned int index_type;
	typedef int grid_index_type;
	typedef math::Rectangle< grid_index_type > RectangleType;
	typedef typename SGCTraits::coord_float_type coord_float_type;
	typedef typename SGCTraits::matrix_float_type matrix_float_type;
	typedef typename SGCTraits::triangle_index_type triangle_index_type;
	typedef VertexSet VertexSetType;
	typedef TriangleSet< triangle_index_type > TriangleSetType;
	typedef math::Vector2< coord_float_type > Vector2Type;
	typedef math::Vector3< coord_float_type > Vector3Type;
	typedef math::Vector2< grid_index_type > GridVectorType;
	typedef std::deque< grid_index_type > GridOffsetList;

	// triangle patchs
	struct Sector
	{
		// triangles in this sector
		typename TriangleSetType::ptr tris;
		// xy offset of this patch relative to 
		index_type xoffs, yoffs;

		Sector() : tris(), xoffs(0), yoffs(0) {}
	};

	typedef std::vector< Sector > SectorList;
	typedef typename SectorList::iterator SectorIterator;
	typedef typename SectorList::const_iterator ConstSectorIterator;

	// vertex grid data
	VertexSetType::ptr verts;

	SectorList sectors;
	//// matrix representing 
	//Matrix4< matrix_float_type > _matrix;

	// angle (radians) between each vertex wrt the origin
	double segmentAngle;

	// offset from next highest level in our units
	GridVectorType offs;
	// offset from clip-map origin (arbitrary, use initial creation location) in our units
	GridVectorType offsGlobal;
	// current error in level (i.e. update movement required during next update)
	GridVectorType error;

	// offsets of vertices
	GridOffsetList xOffsets, yOffsets;

	// index of this level in the clip map
	index_type n;
	Vector3Type center;
	Vector2Type gridCameraCenter;
	//index_type _size;
	//// active region of this level
	RectangleType activeRegion;
	Material::ptr material;
	glbase::Texture texture;
	glbase::Texture normalmap;
	GridVectorType staticOffset;
	FramebufferObject::ptr fbo;
	unsigned int fboAttachment;

#pragma pack(push)
#pragma pack(4)
	struct SGCVertexDef
	{
		Vector3Type pos;
		Vector2Type heightspl;
		Vector2Type gridxy;

		SGCVertexDef() {}
		SGCVertexDef(const Vector3Type& pos_, const Vector2Type& heightspl_, const Vector2Type& gridxy_)
			: pos(pos_), heightspl(heightspl_), gridxy(gridxy_) {}
		SGCVertexDef(const SGCVertexDef& from) : pos(from.pos), heightspl(from.heightspl), gridxy(from.gridxy) {}

		SGCVertexDef operator+(const SGCVertexDef &other) const
		{
			return SGCVertexDef(pos + other.pos, heightspl + other.heightspl, gridxy + other.gridxy);
		}

		SGCVertexDef operator-(const SGCVertexDef &other) const
		{
			return SGCVertexDef(pos - other.pos, heightspl - other.heightspl, gridxy - other.gridxy);
		}

		SGCVertexDef operator*(coord_float_type f) const
		{
			return SGCVertexDef(pos * f, heightspl * f, gridxy * f);
		}

		//SGCVertexDef() : pos(), weights() {}
		//math::Vector3< float > normal;
	};
#pragma pack(pop)

	std::vector< SGCVertexDef > vertices;

//public:
	SGCLevel() 
		: verts(), 
		sectors(), 
		segmentAngle(0), 
		offs(), 
		offsGlobal(), 
		error(), 
		xOffsets(), yOffsets(),
		n(0), 
		center(),
		gridCameraCenter(),
		activeRegion(0, 0, 0, 0),
		material(),
		texture(),
		normalmap(),
		staticOffset(),
		fbo(),
		fboAttachment(0),
		vertices()
	{}

	SectorIterator beginSectors()
	{
		return sectors.begin();
	}

	ConstSectorIterator beginSectors() const
	{
		return sectors.begin();
	}

	SectorIterator endSectors()
	{
		return sectors.end();
	}

	ConstSectorIterator endSectors() const
	{
		return sectors.end();
	}
};


// encapsulates an entire spherical geometry clip map
template < 
	class FloatType, 
	class TriangleIndexType, 
	class SGCTraits = DefaultSGCTraits< FloatType, TriangleIndexType > 
>
struct SGClipMap
{
	typedef typename SGCTraits::coord_float_type coord_float_type;
	typedef typename SGCTraits::matrix_float_type matrix_float_type;
	typedef unsigned int index_type;

	typedef SGCLevel< FloatType, TriangleIndexType, SGCTraits > SGCLevelType;
	typedef typename SGCLevelType::grid_index_type grid_index_type;
	typedef std::vector< SGCLevelType > LevelList;
	typedef typename LevelList::iterator LevelIterator;
	typedef typename LevelList::const_iterator ConstLevelIterator;
	typedef typename SGCTraits::HeightDataProviderType HeightDataProviderType;

	typedef math::Vector3< coord_float_type > Vector3Type;	
	typedef math::Vector2< coord_float_type > Vector2Type;	
	typedef math::Vector4< coord_float_type > Vector4Type;
	typedef math::Matrix4< matrix_float_type > Matrix4Type;

	typedef typename SGCLevelType::TriangleSetType TriangleSetType;
	typedef typename SGCLevelType::SectorIterator SectorIterator;
	typedef typename SGCLevelType::ConstSectorIterator ConstSectorIterator;

	typedef typename SGCTraits::GroupType GroupType;

	typedef typename SGCLevelType::SGCVertexDef SGCVertexDef;

	typedef transform::Camera<matrix_float_type, typename SGCTraits::MatrixTraits> CameraType;

	// settings
	// distance (d) to create first level (l) at (distance to create each subsequent level can be calculated: pow(0.5, l) * d )
	// size (n) (number of verts per side) of each level where n. verts = 1 + pow(2, n)
	// radius (r) of sphere on which the sgc is to be projected
	// center of sphere is assumed to be the origin so all patches are generated relative to
	// the center

private:
	coord_float_type _createDistance;
	grid_index_type _levelSize;
	grid_index_type _sectorsPerLevelSide;
	grid_index_type _sectorSize;
	index_type _maxLevels;
	index_type _currLevels;
	coord_float_type _radius;
	LevelList _levels;
	VertexSpec _vertexSpec;
	Vector3Type _verticalPlaneNormal, _horizontalPlaneNormal;
	GroupType* _parent;
	Shader::ptr _shader;
	Material::ptr _textureMaterial;
	Shader::ptr _textureShader;
	std::vector<glbase::Texture::ptr> _terrainTextures;
	std::vector<FramebufferObject::ptr> _fbos;
	unsigned int _normalsPerVert;
	float _invNormalsPerVert;
	unsigned int _drawVertsSize;
	typename CameraType::ptr _camera;
	//FramebufferObject fbo;

	bool _fboBound;

	typename HeightDataProviderType::ptr _heightData;

public:
	SGClipMap(GroupType* parent = NULL, typename HeightDataProviderType::ptr heightData = HeightDataProviderType::ptr()) 
		: _createDistance(0), 
		_levelSize(0), 
		_sectorsPerLevelSide(0), 
		_sectorSize(0),
		_maxLevels(0), 
		_currLevels(0), 
		_radius(0), 
		_levels(), 
		_vertexSpec(), 
		_verticalPlaneNormal(), 
		_horizontalPlaneNormal(), 
		_parent(parent),
		_heightData(heightData),
		_fbos(),
		_fboBound(false),
		_normalsPerVert(2),
		_invNormalsPerVert(0.5),
		_drawVertsSize(0),
		_camera()
	{
		_vertexSpec.append(VertexData::PositionData, sizeof(coord_float_type), 3, (sizeof(coord_float_type) == sizeof(float))? VertexElementType::Float : VertexElementType::Double);
		_vertexSpec.append(VertexData::TexCoord0, sizeof(coord_float_type), 4, (sizeof(coord_float_type) == sizeof(float))? VertexElementType::Float : VertexElementType::Double);
		//_vertexSpec.append(VertexData::NormalData, sizeof(float) * 3);
		assert(_vertexSpec.vertexSize() == sizeof(SGCVertexDef));
	}

	void setHeightData(typename HeightDataProviderType::ptr heightData)
	{
		_heightData = heightData;
	}
	
	ConstLevelIterator beginLevels() const { return _levels.begin(); }
	ConstLevelIterator endLevels() const { return _levels.begin() + _currLevels; }

	const VertexSpec& vertexSpec() const { return _vertexSpec; }

	template < class TTItr > 
	bool create(grid_index_type levelSize, grid_index_type sectorsPerLevelSide, index_type maxLevels, coord_float_type segmentSize, coord_float_type createDistance, coord_float_type radius, TTItr beginTerrainTextureNames, TTItr endTerrainTextureNames, typename CameraType::ptr camera)
	{
		assert(_parent != NULL);

		_camera = camera;
		_createDistance = createDistance;
		_levelSize = levelSize;
		_maxLevels = maxLevels;
		_currLevels = 0;
		_radius = radius;

		// levelSize must be evenly divisible by the segments per side
		if((levelSize-1) % sectorsPerLevelSide != 0)
			return false;

		_sectorsPerLevelSide = sectorsPerLevelSide;
		_drawVertsSize = (_levelSize-1) * _invNormalsPerVert + 1;
		_sectorSize = (_levelSize-1) / sectorsPerLevelSide;

		// convert segment size from distance units to angular units
		double angularSegmentSize = math::rad_to_deg(atan(segmentSize / radius));
		// add levels setting their size and index
		_levels.resize(_maxLevels);
		SGCLevelType::index_type n = 0;
		for(LevelIterator lItr = _levels.begin(); lItr != _levels.end(); ++lItr, ++n, angularSegmentSize *= 0.5)
		{
			lItr->n = n;
			lItr->segmentAngle = angularSegmentSize;
		}

		// setup materials and shaders
		_shader = ShaderManager::globalManager().createShader("sgcshader");
		_shader->create(
			"void vert(float3 INposition : POSITION,"
			"	float4 INheightsAndGridxy : TEXCOORD0," 
			"	uniform float4x4 modelViewProj,"
			"	uniform float2 gridCameraCenter,"
			"	uniform float2 gridOffset,"
			"	uniform float gridSize,"
			"	uniform float transitionSize,"
			"	uniform float2 parentOffset,"
			"   uniform float3 light0Dir,"
			"	out float4 OUTposition : POSITION,\n"
			"	out float3 OUTuvp : TEXCOORD0,\n"
			"	out float2 OUTuvl : TEXCOORD1,\n"
			"	out float3 OUTldir : TEXCOORD2)\n"
			"{\n"
			"	float2 relxy = INheightsAndGridxy.zw - gridOffset;\n"
			"   float2 w2 = saturate(((gridSize-transitionSize-1)*0.5 - abs((gridSize*0.5 + gridCameraCenter) - relxy)) / transitionSize);\n"
			"	float w = w2.x * w2.y;\n"
			"   float3 ploc = INposition * INheightsAndGridxy.x;\n"
			"   float3 lloc = INposition * INheightsAndGridxy.y;\n"
			"	OUTposition = mul(modelViewProj, float4(ploc * (1.0 - w) + lloc * w, 1.0));\n" 
			"	OUTuvl = (INheightsAndGridxy.zw/*-parentOffset*/) / gridSize;\n"
			"	OUTuvp.xy = ((INheightsAndGridxy.zw)* 0.5) / gridSize;\n"
			"	OUTuvp.z = w;\n"
			"	OUTldir = light0Dir;\n"
			"}\n"
			"void frag(float3 color : COLOR, float3 uvp : TEXCOORD0, float2 uvl : TEXCOORD1, float3 ldir : TEXCOORD2, uniform sampler2D texmap, uniform sampler2D normalmap, uniform sampler2D ptexmap, uniform sampler2D pnormalmap, uniform float3 ambient, out float4 destPixel : COLOR)\n"
			"{\n"
			"	float3 normal = (f3tex2D(normalmap, uvl) - 0.5) * 2;"
			"	float3 normalp = (f3tex2D(pnormalmap, uvp.xy) - 0.5) * 2;"
			"	float lighting = saturate(dot(ldir, normal)) * uvp.z + saturate(dot(ldir, normalp)) * (1 - uvp.z);\n"
			"	destPixel = float4(ambient + lighting * (f3tex2D(texmap, uvl) * uvp.z + f3tex2D(ptexmap, uvp.xy) * (1 - uvp.z)), 1.0);\n"
			"}\n"
			"float4x4 ModelViewProj : ModelViewProjectionMat;\n"
			"float3 Ambient;\n"
			"float2 GridCameraCenter;\n"
			"float2 GridOffset;\n"
			"float GridSize;\n"
			"float TransitionSize;\n"
			"float2 ParentOffset;\n"
			"float3 Light0Dir;\n"
			"sampler2D LevelTexture = sampler_state\n"
			"{minFilter = Linear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
			"sampler2D LevelNormalMap = sampler_state\n"
			"{minFilter = Linear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
			"sampler2D ParentTexture = sampler_state\n"
			"{minFilter = Linear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
			"sampler2D ParentNormalMap = sampler_state\n"
			"{minFilter = Linear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
			"technique nv40\n"
			"{\n"
			"	pass\n"
			"	{\n"
			"		DepthTestEnable = true;\n"
			"		CullFaceEnable = false;\n"
			"		VertexProgram =	compile vp40 vert(ModelViewProj, GridCameraCenter, GridOffset, GridSize, TransitionSize, ParentOffset, Light0Dir);\n"
			"		FragmentProgram = compile fp40 frag(LevelTexture, LevelNormalMap, ParentTexture, ParentNormalMap, Ambient);\n"
			"	}\n"
			"}\n" 
		);
		

		_textureShader = ShaderManager::globalManager().createShader("sgctextureshader");
		_textureShader->create(
			"void vert(float3 INposition : POSITION,\n"
			"	float2 INuv : TEXCOORD0,\n" 
			"	float4 INweightsA : TEXCOORD1,\n"
			"	float4 INweightsB : TEXCOORD2,\n"
			"	uniform float4x4 modelViewProj,\n"
			"	out float4 OUTposition : POSITION,\n"
			"	out float2 OUTuv : TEXCOORD0,\n"
			"	out float4 OUTweightsA : TEXCOORD1,\n"
			"	out float4 OUTweightsB : TEXCOORD2)\n"
			"{\n"
			"	OUTposition = mul(modelViewProj, float4(INposition, 1.0));\n" 
			"	OUTuv = INuv; OUTweightsA = INweightsA; OUTweightsB = INweightsB;\n"
			"}\n"
			"void frag(float2 INuv : TEXCOORD0,\n"
			"	float4 INweightsA : TEXCOORD1,\n"
			"	float4 INweightsB : TEXCOORD2,\n"
			"	uniform sampler2D terr0 : TEXUNIT0, uniform sampler2D terr1 : TEXUNIT1,\n"
			"	out float4 OUTcolor : COLOR)\n"
			"{\n"
			"	OUTcolor = float4(INweightsA.x * f3tex2D(terr0, INuv) + INweightsA.y * f3tex2D(terr1, INuv), 1.0);\n"
			"}\n"
			"float4x4 ModelViewProj : ModelViewProjectionMat;\n"
			"sampler2D Terrain0 = sampler_state\n"
			"{\nminFilter = Linear;\n magFilter = Linear;\n WrapS = Repeat;\n WrapT = Repeat;\n };\n"
			"sampler2D Terrain1 = sampler_state\n"
			"{minFilter = Linear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
// 			"sampler2D Terrain2 = sampler_state\n"
// 			"{minFilter = LinearMipMapLinear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
// 			"sampler2D Terrain3 = sampler_state\n"
// 			"{minFilter = LinearMipMapLinear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
// 			"sampler2D Terrain4 = sampler_state\n"
// 			"{minFilter = LinearMipMapLinear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
// 			"sampler2D Terrain5 = sampler_state\n"
// 			"{minFilter = LinearMipMapLinear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
// 			"sampler2D Terrain6 = sampler_state\n"
// 			"{minFilter = LinearMipMapLinear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
// 			"sampler2D Terrain7 = sampler_state\n"
// 			"{minFilter = LinearMipMapLinear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
// 			"sampler2D Terrain8 = sampler_state\n"
// 			"{minFilter = LinearMipMapLinear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
// 			"sampler2D Terrain9 = sampler_state\n"
// 			"{minFilter = LinearMipMapLinear; magFilter = Linear; WrapS = Repeat; WrapT = Repeat; };\n"
			"technique nv40\n"
			"{\n"
			"	pass\n"
			"	{\n"
			"		DepthTestEnable = false;\n"
			"		CullFaceEnable = false;\n"
			"		VertexProgram =	compile vp40 vert(ModelViewProj);\n"
			"		FragmentProgram = compile fp40 frag(Terrain0, Terrain1);\n"
			"	}\n"
			"}\n" 
		);

		// potential terrain rules:
		// height
		// normal angle to vertical (inclination)
		// 
		// initialize material for rendering textures
		_textureMaterial.reset(new Material());
		_textureMaterial->setName("sgctexturemat");
		_textureMaterial->setShader(_textureShader);

		// load terrain textures
		_terrainTextures.clear();
		unsigned int idx = 0;
		for(TTItr itr = beginTerrainTextureNames; itr != endTerrainTextureNames; ++itr, ++idx)
		{
			glbase::Texture::ptr tex(new glbase::Texture());
			tex->load(*itr);
			std::stringstream ss;
			ss << "Terrain" << idx;
			// attach texture to terrain material
			_textureMaterial->textureInputMap()[ss.str()] = tex->handle();
			_terrainTextures.push_back(tex);
		}
		
		// initialize fbos for rendering level textures
		unsigned int numFbos = static_cast<unsigned int>(std::ceil(_maxLevels / static_cast<float>(FramebufferObject::GetMaxColorAttachments())));
		for(unsigned int idx = 0; idx < numFbos; ++idx)
		{
			FramebufferObject::ptr newFbo(new FramebufferObject());
			_fbos.push_back(newFbo);
		}
		return true;
	}

	template < class TFloatType >
	void update(TFloatType t)
	{
		_fboBound = false;

		math::Vector3<matrix_float_type> localCameraLocation = _parent->localise(Vector3Type(_camera->transform().getColumnVector(3)));

		// calculate distance to surface of sphere from camera
		coord_float_type camDist = localCameraLocation.length() - getHeightAndRadius(localCameraLocation.normal());

		coord_float_type levelDrawDist = _createDistance;

		std::vector<bool> changed(_levels.size(), false);

		typename SGCLevelType::GridVectorType lastError;

		if(_currLevels > 0)
		{
			// calculate movement of each level
			for(index_type l = 0; l < _currLevels; ++l)
			{
				calculateGridError(_levels[l], localCameraLocation.normal());
				//_levels[l].error = calculateError(_levels[l], localCameraLocation.normal());
			}
			lastError = _levels[_currLevels-1].error;

			// apply x movement
			for(index_type l = 0; l < _currLevels; ++l)
			{
				SGCLevelType& level = _levels[l];
				//GridVectorType actualError = level.error * _invNormalsPerVert;
				changed[l] = changed[l] || (level.error.lengthSquared() != 0);
				if(abs(level.error.x) > (_levelSize-1)*0.5 || abs(level.error.y) > (_levelSize-1)*0.5)
				{
					_currLevels = l;
				}
				else
				{
					fixError(level);
				}
				//updateX(_levels[l]);
				//updateY(_levels[l]);
			}
		}

		// delete all unwanted levels
		for(index_type l = 0; l < _levels.size(); ++l, levelDrawDist *= 0.5)
		{
			// if the camera is closer than the creation distance and the level does not already exist then create it
			if(camDist <= levelDrawDist && _currLevels < l+1 && (abs(lastError.x) < (_levelSize-1)*0.1 && abs(lastError.y) < (_levelSize-1)*0.1))
			{
				// if we are about to create the base patch (lowest LOD patch)
				if(_currLevels == 0)
					calculateHVVectors(localCameraLocation);
				else
					_levels[l].offs = _levels[l-1].offs * 2 + _levelSize * 0.5;
				_currLevels = l+1;
				createLevel(_levels[l], localCameraLocation.normal());
				changed[l] = true;
				// scale up the last error value so it corresponds with the new level, and can be used in judging whether to create a further level
				lastError *= 2;
			}
			else if(camDist > levelDrawDist && _currLevels >= l+1)
			{
				// make sure it is not updated or drawn		
				_currLevels = l;
				changed[l] = true;
			}
		}

		// update shader settings

		for(index_type l = 0; l < _currLevels; ++l)
		{
			SGCLevelType& level = _levels[l];
			level.gridCameraCenter = calculateContinuousError(level, localCameraLocation);
			level.material->vector2fInputMap()["GridCameraCenter"] = level.gridCameraCenter * _invNormalsPerVert;
			//if(changed[l])
			//{
				level.material->vector2fInputMap()["GridOffset"] = level.offs * _invNormalsPerVert;
			//if(l != 0)
				//level.material->vector2fInputMap()["ParentOffset"] = level.offs - _levels[l-1].offs * 2;// * 0;//_invNormalsPerVert;
				//if(l > 0)
				//	level.material->vector2fInputMap()["ParentOffset"] = math::Vector2f((_levelSize-1)*0.5, (_levelSize-1)*0.5);//(getParentX(_levels[l], _levels[l-1], 0) * 2, getParentY(_levels[l], _levels[l-1], 0) * 2);
			//}
		}


// 		// apply y movement
// 		for(index_type l = 0; l < _currLevels; ++l)
// 		{
// 		}

		// must create sectors seperately at the end for each level as a hole must be made in a level to fit its child, 
		// which must be fully updated, or there will be gaps
		for(index_type l = 0; l < _currLevels; ++l)
		{
			if(changed[l] || (l < _levels.size()-1 && changed[l+1]))
			{
				createSectors(_levels[l], 0, 0, _levelSize, _levelSize);
			}
		}

		if(_fboBound)
		{
			FramebufferObject::Disable();
			_fboBound = false;
		}
	}

private:

	void fixError(SGCLevelType& level)
	{
		//if(abs(level.error.x) >= _levelSize-1 || abs(level.error.y) >= _levelSize-1)
		//{
		//	level.offs += level.error;

		//	setActiveRegion(level);
		//	//if(level.error.x != 0 && level.n < _currLevels-1)
		//	//	setActiveRegion(_levels[level.n+1]);

		//	if(level.error.x > 0) 
		//		for(grid_index_type i = 0; i < level.error.x; i++)	{
		//			level.xOffsets.push_back(level.xOffsets.front());
		//			level.xOffsets.pop_front();
		//		}
		//	else for(grid_index_type i = 0; i < -level.error.x; i++) {
		//		level.xOffsets.push_front(level.xOffsets.back());
		//		level.xOffsets.pop_back();
		//	}

		//	if(level.error.y > 0) 
		//		for(grid_index_type i = 0; i < level.error.y; i++)	{
		//			level.yOffsets.push_back(level.yOffsets.front());
		//			level.yOffsets.pop_front();
		//		}
		//	else for(grid_index_type i = 0; i < -level.error.y; i++) {
		//		level.yOffsets.push_front(level.yOffsets.back());
		//		level.yOffsets.pop_back();
		//	}


		//	updateCenter(level);

		//	recalculateVerts(level, 0, 0, _levelSize, _levelSize);

		//	level.error.x = level.error.y = 0;
		//}
		//else
		{
			updateX(level);
			updateY(level);
		
		}
	}


	template < class FType > 
	FType mod_abs(FType val, FType mod)
	{
		FType result = val % mod;
		if(result < 0)
			result = mod + result;
		return result;
	}

	void updateX(SGCLevelType& level)
	{
		// ensure updates are in multiples of the number of normals per verts
		//grid_index_type drawError = level.error.x * _invNormalsPerVert;
		//level.error.x = drawError * _normalsPerVert;

		level.offs.x += level.error.x;
		setActiveRegion(level);
	
		if(level.error.x > 0)
		{
			for(grid_index_type i = 0; i < level.error.x; i++)
			{
				level.xOffsets.push_back(level.xOffsets.front());
				level.xOffsets.pop_front();
			}

			updateCenter(level);

			recalculateVerts(level, (_levelSize - 1) - level.error.x, 0, _levelSize, _levelSize);
			// update the discarded piece of texture with new data
			updateTexture(level, level.offs.x + ((_levelSize-1) - level.error.x)/* - 1*/, 
				level.offs.y, level.offs.x + (_levelSize-1), level.offs.y + _levelSize, false);
			updateNormalMap(level, level.offs.x + ((_levelSize-1) - level.error.x) - 1, 
				level.offs.y, level.offs.x + (_levelSize-1), level.offs.y + _levelSize);
		}
		else if(level.error.x < 0)
		{
			grid_index_type abserrx = -level.error.x;
			for(grid_index_type i = 0; i < abserrx; i++)
			{
				level.xOffsets.push_front(level.xOffsets.back());
				level.xOffsets.pop_back();
			}

			updateCenter(level);

			recalculateVerts(level, 0, 0, abserrx+1, _levelSize);

			updateTexture(level, level.offs.x,
				level.offs.y, level.offs.x - level.error.x/* + 1*/, level.offs.y + _levelSize, false);
			updateNormalMap(level, level.offs.x,
				level.offs.y, level.offs.x - level.error.x + 1, level.offs.y + _levelSize);
		}
		level.error.x = 0;
	}

	void updateCenter( SGCLevelType &level )
	{
		assert(math::intersects(typename SGCLevelType::GridVectorType((_levelSize-1)*0.5, (_levelSize-1)*0.5), level.activeRegion).occured);
		grid_index_type xcenter = offsetX(level, static_cast<grid_index_type>((_levelSize-1)*0.5));
		grid_index_type ycenter = offsetY(level, static_cast<grid_index_type>((_levelSize-1)*0.5));
		level.center = getVertex(level, calcVertexOffset(xcenter, ycenter)).pos.normal();
		recalculateHVVectors(level.center);
	}

	void updateY(SGCLevelType& level)
	{
		// ensure updates are in multiples of the number of normals per verts
		//grid_index_type drawError = level.error.y * _invNormalsPerVert;
		//level.error.y = drawError * _normalsPerVert;

		level.offs.y += level.error.y;
		setActiveRegion(level);
		
		if(level.error.y > 0)
		{
			for(grid_index_type i = 0; i < level.error.y; i++)
			{
				level.yOffsets.push_back(level.yOffsets.front());
				level.yOffsets.pop_front();
			}

			updateCenter(level);

			recalculateVerts(level, 0, (_levelSize-1) - level.error.y, _levelSize, _levelSize);
			updateTexture(level, level.offs.x, 
				level.offs.y + ((_levelSize-1) - level.error.y)/* - 1*/, level.offs.x + _levelSize, level.offs.y + (_levelSize - 1));
			updateNormalMap(level, level.offs.x, 
				level.offs.y + ((_levelSize-1) - level.error.y) - 1, level.offs.x + _levelSize, level.offs.y + (_levelSize - 1));
		}
		else if(level.error.y < 0)
		{
			grid_index_type abserry = -level.error.y;
			for(grid_index_type i = 0; i < abserry; i++)
			{
				level.yOffsets.push_front(level.yOffsets.back());
				level.yOffsets.pop_back();
			}

			updateCenter(level);

			recalculateVerts(level, 0, 0, _levelSize, abserry+1);
			updateTexture(level, level.offs.x,
				level.offs.y, level.offs.x + _levelSize, level.offs.y - level.error.y/* + 1*/);
			updateNormalMap(level, level.offs.x,
				level.offs.y, level.offs.x + _levelSize, level.offs.y - level.error.y + 1);
		}
		level.error.y = 0;
	}

	Vector3Type transformVec(const Matrix4Type& mat, const Vector3Type& vec) const
	{
		return Vector3Type(mat * (Vector4Type(vec) + Vector4Type::WAxis));
	}

	grid_index_type offsetX(const SGCLevelType& level, grid_index_type idx) const
	{
		return level.xOffsets[idx];//MY_MIN(MY_MAX(0, idx), _levelSize-1)];
	}

	grid_index_type offsetY(const SGCLevelType& level, grid_index_type idx) const
	{
		return level.yOffsets[idx];//MY_MIN(MY_MAX(0, idx), _levelSize-1)];
	}


	typename SGCLevelType::index_type calcVertexOffset(grid_index_type x, grid_index_type y) const
	{
		return y * _levelSize + x;
	}

	grid_index_type xToDrawOffset(const SGCLevelType& level, grid_index_type x)
	{
		return mod_abs<grid_index_type>((x + level.offs.x) * _invNormalsPerVert, _drawVertsSize);
	}

	grid_index_type yToDrawOffset(const SGCLevelType& level, grid_index_type y)
	{
		return mod_abs<grid_index_type>((y + level.offs.y) * _invNormalsPerVert, _drawVertsSize);
	}

	typename SGCLevelType::index_type calcDrawVertexOffset(/*const SGCLevelType& level, */grid_index_type x, grid_index_type y) const
	{
		//grid_index_type fx = mod_abs<grid_index_type>((x + level.offs.x) * _invNormalsPerVert, _drawVertsSize);
		//grid_index_type fy = mod_abs<grid_index_type>((y + level.offs.y) * _invNormalsPerVert, _drawVertsSize);
		return y * _drawVertsSize + x;//yToDrawOffset(level, y) * _drawVertsSize + xToDrawOffset(level, x);
		//return static_cast<typename SGCLevelType::index_type>((static_cast<float>(y) * _invNormalsPerVert) * static_cast<float>(_drawVertsSize) + static_cast<float>(x) * _invNormalsPerVert);
	}

	// return the equivalent x coordinate of j in the parents units
	grid_index_type getParentX(const SGCLevelType& level, const SGCLevelType& parent, grid_index_type j)
	{
		return static_cast<grid_index_type>((j + level.offs.x - parent.offs.x*2)*0.5);
	}

	grid_index_type getParentY(const SGCLevelType& level, const SGCLevelType& parent, grid_index_type i)
	{
		return static_cast<grid_index_type>((i + level.offs.y - parent.offs.y*2)*0.5);
	}

	grid_index_type getChildX(const SGCLevelType& level, const SGCLevelType& child, grid_index_type j)
	{
		return static_cast<grid_index_type>((j + level.offs.x) * 2.0 - child.offs.x);
	}

	grid_index_type getChildY(const SGCLevelType& level, const SGCLevelType& child, grid_index_type i)
	{
		return static_cast<grid_index_type>((i + level.offs.y) * 2.0 - child.offs.y);
	}

	void recalculateVerts(SGCLevelType& level, grid_index_type startx, grid_index_type starty, grid_index_type endx, grid_index_type endy)
	{
		typedef typename SGCLevelType::index_type idx_type;
		//typedef math::Matrix4<> Matrix4Type;
		// if it is base level then all calculation is done from scratch
		startx = MY_MAX(startx, level.activeRegion.left);
		starty = MY_MAX(starty, level.activeRegion.bottom);
		endx = MY_MIN(endx, level.activeRegion.right);
		endy = MY_MIN(endy, level.activeRegion.top);

		assert(startx % _normalsPerVert == 0);
		assert((endx-1) % _normalsPerVert == 0);
		assert(starty % _normalsPerVert == 0);
		assert((endy-1) % _normalsPerVert == 0);

		if(level.n == 0)
		{
			Vector3Type center = level.center;
			coord_float_type angle = level.segmentAngle;

			grid_index_type dy = (level.offs.y + starty) * _invNormalsPerVert;
			for(grid_index_type y = starty; y < endy; ++y)
			{
				coord_float_type yangle = angle * ((y/* + static_cast<coord_float_type>(level.offs.y)*/) - ((_levelSize-1)*0.5));
				Matrix4Type yrot( math::rotate_axis_angle(_verticalPlaneNormal, yangle) );

				Vector3Type centeroffs(transformVec(yrot, center));

				Vector3Type yaxis(transformVec(yrot, _horizontalPlaneNormal));

				grid_index_type offsy = offsetY(level, y);
				grid_index_type offsyd = yToDrawOffset(level, y);

				// if end is less than start then the range wraps
				if((y + level.offs.y) % _normalsPerVert == 0)
				{
					// calculate the start and end vertex buffer positions and dirty the buffer
					idx_type startoffsx = xToDrawOffset(level, startx);//mod_abs((startx + level.offs.x) * _invNormalsPerVert, _drawVertsSize);//offsetX(level, startx);
					idx_type endoffsx = xToDrawOffset(level, endx-1);//offsetX(level, endx-1);
					if(endoffsx < startoffsx)
					{
						idx_type vbstart = calcDrawVertexOffset(0, offsyd);//calcVertexOffset(0, offsy*_invNormalsPerVert);
						idx_type vbend = calcDrawVertexOffset(endoffsx, offsyd);//calcVertexOffset(endoffsx*_invNormalsPerVert, offsy*_invNormalsPerVert);
						level.verts->dirty(vbstart, vbend+1);
						vbstart = calcDrawVertexOffset(startoffsx, offsyd);//calcVertexOffset(startoffsx*_invNormalsPerVert, offsy*_invNormalsPerVert);
						vbend = calcDrawVertexOffset(_drawVertsSize-1, offsyd);//calcVertexOffset(_drawVertsSize-1, offsy*_invNormalsPerVert);
						level.verts->dirty(vbstart, vbend+1);
					}
					else
					{
						idx_type vbstart = calcDrawVertexOffset(startoffsx, offsyd);//calcVertexOffset(startoffsx*_invNormalsPerVert, offsy*_invNormalsPerVert);
						idx_type vbend = calcDrawVertexOffset(endoffsx, offsyd);//calcVertexOffset(endoffsx*_invNormalsPerVert, offsy*_invNormalsPerVert);
						level.verts->dirty(vbstart, vbend+1);
					}
				}
				//idx_type vbstart = calcBufferOffset(0, offsy);
				//idx_type vbend = calcBufferOffset(_levelSize-1, offsy);
				//level.verts->dirty(vbstart, vbend+1);


				grid_index_type dx = (level.offs.y + starty) * _invNormalsPerVert;
				for(grid_index_type x = startx; x < endx; ++x)
				{
					grid_index_type offsx = offsetX(level, x);
					grid_index_type offsxd = xToDrawOffset(level, x);
					idx_type offs = calcVertexOffset(offsx, offsy);
					coord_float_type xangle = angle * ((x/* + static_cast<coord_float_type>(level.offs.x)*/) - ((_levelSize-1)*0.5));
					Matrix4Type xrot(math::rotate_axis_angle(_horizontalPlaneNormal, xangle));

					Vector3Type newpoint(transformVec(xrot, centeroffs));

					//addNoiseAndRadius(newpoint);
					coord_float_type h = getHeightAndRadius(newpoint);

					SGCVertexDef newVert(newpoint, Vector2Type(_radius, h), Vector2Type(level.offs.x + x, level.offs.y + y) * _invNormalsPerVert);
					setVertex(level, offs, newVert);
					if(x % _normalsPerVert == 0 && y % _normalsPerVert == 0)
					{
						//newVert.gridxy *= _invNormalsPerVert;
						//assert(offsx % 2 == 0 && offsy % 2 == 0);
						setDrawVertex(level, calcDrawVertexOffset(offsxd, offsyd), newVert);//offsetX(level, x) * _invNormalsPerVert + offsetY(level, y) * _invNormalsPerVert * _drawVertsSize, newVert);
					}
				}
			}
		}
		// if not base level then we can take certain verts from the level above and interpolate those in between
		// (not only can we, we must, as errors introduced during the wrapping of the clipmap onto a sphere mean that
		// using pure calculation for the higher levels leads to bad results)
		else
		{
			SGCLevelType& parentlevel = _levels[level.n-1];

			for(grid_index_type y = starty; y < endy; ++y)
			{
				grid_index_type offsy = offsetY(level, y);
				if((y + level.offs.y) % _normalsPerVert == 0)
				{
					grid_index_type offsyd = yToDrawOffset(level, y);
					// calculate the start and end vertex buffer positions and dirty the buffer
					idx_type startoffsx = xToDrawOffset(level, startx);//mod_abs((startx + level.offs.x) * _invNormalsPerVert, _drawVertsSize);//offsetX(level, startx);
					idx_type endoffsx = xToDrawOffset(level, endx-1);//offsetX(level, endx-1);
					if(endoffsx < startoffsx)
					{
						idx_type vbstart = calcDrawVertexOffset(0, offsyd);//calcVertexOffset(0, offsy*_invNormalsPerVert);
						idx_type vbend = calcDrawVertexOffset(endoffsx, offsyd);//calcVertexOffset(endoffsx*_invNormalsPerVert, offsy*_invNormalsPerVert);
						level.verts->dirty(vbstart, vbend+1);
						vbstart = calcDrawVertexOffset(startoffsx, offsyd);//calcVertexOffset(startoffsx*_invNormalsPerVert, offsy*_invNormalsPerVert);
						vbend = calcDrawVertexOffset(_drawVertsSize-1, offsyd);//calcVertexOffset(_drawVertsSize-1, offsy*_invNormalsPerVert);
						level.verts->dirty(vbstart, vbend+1);
					}
					else
					{
						idx_type vbstart = calcDrawVertexOffset(startoffsx, offsyd);//calcVertexOffset(startoffsx*_invNormalsPerVert, offsy*_invNormalsPerVert);
						idx_type vbend = calcDrawVertexOffset(endoffsx, offsyd);//calcVertexOffset(endoffsx*_invNormalsPerVert, offsy*_invNormalsPerVert);
						level.verts->dirty(vbstart, vbend+1);
					}
				}
			}
			
			// firstly copy even grid elements
			for(grid_index_type y = starty; y < endy; y+=2)
			{
				grid_index_type offsy = offsetY(level, y);
				grid_index_type offsyd = yToDrawOffset(level, y);
				grid_index_type py = getParentY(level, parentlevel, y);
				grid_index_type hy = offsetY(parentlevel, py);
				for(grid_index_type x = startx; x < endx; x+=2)
				{
					grid_index_type offsxd = xToDrawOffset(level, x);
					idx_type offs = calcVertexOffset(offsetX(level, x), offsy);
					grid_index_type px = getParentX(level, parentlevel, x);
					idx_type offsh = calcVertexOffset(offsetX(parentlevel, px), hy);

					const SGCVertexDef& vert = getVertex(parentlevel, offsh);
					// both heights are the same for verts shared between both levels
					SGCVertexDef newVert(vert.pos, Vector2Type(vert.heightspl.y, vert.heightspl.y), Vector2Type(level.offs.x + x, level.offs.y + y) * _invNormalsPerVert);
					if(x % _normalsPerVert == 0 && y % _normalsPerVert == 0)
					//{
					//	// all sub level verts can be sourced from parent therefore we need to calculate the correct interpolation heights here from our nearest neighbours
					//	if(x % (_normalsPerVert*2) != 0 && y % (_normalsPerVert*2) != 0)
					//	{
					//		// odd x and odd y
					//		SGCVertexDef& v0 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x - _normalsPerVert), yToDrawOffset(level, y - _normalsPerVert)));
					//		SGCVertexDef& v1 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x + _normalsPerVert), yToDrawOffset(level, y + _normalsPerVert)));
					//		newVert.heightspl.x = v0.heightspl.y * 0.5 + v1.heightspl.y * 0.5;
					//	}
					//	else if(x % (_normalsPerVert*2) != 0)
					//	{
					//		// odd x even y
					//		SGCVertexDef& v0 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x - _normalsPerVert), yToDrawOffset(level, y)));
					//		SGCVertexDef& v1 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x + _normalsPerVert), yToDrawOffset(level, y)));
					//		newVert.heightspl.x = v0.heightspl.y * 0.5 + v1.heightspl.y * 0.5;
					//	}
					//	else if(y % (_normalsPerVert*2) != 0)
					//	{
					//		// odd y even x
					//		SGCVertexDef& v0 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x), yToDrawOffset(level, y - _normalsPerVert)));
					//		SGCVertexDef& v1 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x), yToDrawOffset(level, y + _normalsPerVert)));
					//		newVert.heightspl.x = v0.heightspl.y * 0.5 + v1.heightspl.y * 0.5;
					//	}
						setDrawVertex(level, calcDrawVertexOffset(offsxd, offsyd), newVert);
					//}
					setVertex(level, offs, newVert);
				}
			}

			for(grid_index_type y = starty; y < endy; y+=2)
			{
				grid_index_type offsy = offsetY(level, y);
				grid_index_type offsyd = yToDrawOffset(level, y);
				//grid_index_type py = getParentY(level, parentlevel, y);
				//grid_index_type hy = offsetY(parentlevel, py);
				for(grid_index_type x = startx; x < endx; x+=2)
				{
					if(x % _normalsPerVert == 0 && y % _normalsPerVert == 0)
					{
						grid_index_type offsxd = xToDrawOffset(level, x);
						//idx_type offs = calcVertexOffset(offsetX(level, x), offsy);
						//grid_index_type px = getParentX(level, parentlevel, x);
						//idx_type offsh = calcVertexOffset(offsetX(parentlevel, px), hy);

						//const SGCVertexDef& vert = getVertex(parentlevel, offsh);
						// both heights are the same for verts shared between both levels
						SGCVertexDef& newVert = getDrawVertex(level, calcDrawVertexOffset(offsxd, offsyd));
						// all sub level verts can be sourced from parent therefore we need to calculate the correct interpolation heights here from our nearest neighbours
						if(x % (_normalsPerVert*2) != 0 && y % (_normalsPerVert*2) != 0)
						{
							// odd x and odd y
							SGCVertexDef& v0 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x + _normalsPerVert), yToDrawOffset(level, y - _normalsPerVert)));
							SGCVertexDef& v1 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x - _normalsPerVert), yToDrawOffset(level, y + _normalsPerVert)));
							newVert.heightspl.x = v0.heightspl.y * 0.5 + v1.heightspl.y * 0.5;
						}
						else if(y % (_normalsPerVert*2) != 0)
						{
							// odd y even x
							SGCVertexDef& v0 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x), yToDrawOffset(level, y - _normalsPerVert)));
							SGCVertexDef& v1 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x), yToDrawOffset(level, y + _normalsPerVert)));
							newVert.heightspl.x = v0.heightspl.y * 0.5 + v1.heightspl.y * 0.5;
						}
						else if(x % (_normalsPerVert*2) != 0)
						{
							// odd x even y
							SGCVertexDef& v0 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x - _normalsPerVert), yToDrawOffset(level, y)));
							SGCVertexDef& v1 = getDrawVertex(level, calcDrawVertexOffset(xToDrawOffset(level, x + _normalsPerVert), yToDrawOffset(level, y)));
							newVert.heightspl.x = v0.heightspl.y * 0.5 + v1.heightspl.y * 0.5;
						}
					}
				}
			}

			for(grid_index_type y = starty; y < endy; y+=_normalsPerVert)
			{
				for(grid_index_type x = startx+_normalsPerVert; x < endx-_normalsPerVert; x+=_normalsPerVert)
				{

				}
			}

			coord_float_type factor = 100;
			// now fill the even y, odd x gaps
			for(grid_index_type y = starty; y < endy; y+=2)
			{
				grid_index_type offsy = offsetY(level, y);
				grid_index_type offsyd = yToDrawOffset(level, y);
				for(grid_index_type x = startx+1; x < endx-1; x+=2)
				{
					grid_index_type offsxd = xToDrawOffset(level, x);
					idx_type offs = calcVertexOffset(offsetX(level, x), offsy);
					idx_type offsxm1 = calcVertexOffset(offsetX(level, x-1), offsy);
					idx_type offsxp1 = calcVertexOffset(offsetX(level, x+1), offsy);

					SGCVertexDef& v0 = getVertex(level, offsxm1);
					SGCVertexDef& v1 = getVertex(level, offsxp1);
					SGCVertexDef newpoint;
					//	newpoint = (( + getVertex(level, offsxp1)) * 0.5);
					newpoint.pos = (v0.pos * factor + v1.pos * factor).normal();//newpoint.pos.normal();
					newpoint.heightspl = v0.heightspl * 0.5 + v1.heightspl * 0.5;//0.5;
					newpoint.heightspl.x = newpoint.heightspl.y;
					newpoint.heightspl.y = getHeightAndRadius(newpoint.pos);
					newpoint.gridxy = Vector2Type(level.offs.x + x, level.offs.y + y) * _invNormalsPerVert;
					//SGCVertexDef newpoint = ((getVertex(level, offsxm1) + getVertex(level, offsxp1)) * 0.5);
					//newpoint.pos = newpoint.pos.normal();
					//newpoint.heightspl.x = newpoint.heightspl.y;
					//newpoint.heightspl.y = getHeightAndRadius(newpoint.pos);
					//newpoint.gridxy = Vector2Type(level.offs.x + x, level.offs.y + y);
					
					//addNoiseAndRadius(newpoint);
					if(x % _normalsPerVert == 0 && y % _normalsPerVert == 0)
					{
						// correct height to match parents height taking into account normals per vertex
						idx_type offsxm1d = calcDrawVertexOffset(xToDrawOffset(level, x-_normalsPerVert), offsyd);
						idx_type offsxp1d = calcDrawVertexOffset(xToDrawOffset(level, x+_normalsPerVert), offsyd);
						SGCVertexDef& v0d = getDrawVertex(level, offsxm1d);
						SGCVertexDef& v1d = getDrawVertex(level, offsxp1d);
						newpoint.heightspl.x = v0d.heightspl.x * 0.5 + v1d.heightspl.x * 0.5; newpoint.heightspl.y = 1;
						setDrawVertex(level, calcDrawVertexOffset(offsxd, offsyd), newpoint);
					}
					setVertex(level, offs, newpoint);
				}
			}
			// now fill the odd y, even x gaps
			for(grid_index_type y = starty+1; y < endy-1; y+=2)
			{
				grid_index_type offsy = offsetY(level, y);
				grid_index_type offsyd = yToDrawOffset(level, y);
				grid_index_type offsym1 = offsetY(level, y-1);
				grid_index_type offsyp1 = offsetY(level, y+1);
				grid_index_type offsym1d = yToDrawOffset(level, y - _normalsPerVert);
				grid_index_type offsyp1d = yToDrawOffset(level, y + _normalsPerVert);
				for(grid_index_type x = startx; x < endx; x+=2)
				{
					grid_index_type offsxd = xToDrawOffset(level, x);
					grid_index_type goffsx = offsetX(level, x);
					idx_type offs = calcVertexOffset(goffsx, offsy);
					idx_type offsxm1 = calcVertexOffset(goffsx, offsym1);
					idx_type offsxp1 = calcVertexOffset(goffsx, offsyp1);

					SGCVertexDef& v0 = getVertex(level, offsxm1);
					SGCVertexDef& v1 = getVertex(level, offsxp1);
					SGCVertexDef newpoint;
					//	newpoint = (( + getVertex(level, offsxp1)) * 0.5);
					newpoint.pos = (v0.pos * factor + v1.pos * factor).normal();//newpoint.pos.normal();
					newpoint.heightspl = v0.heightspl * 0.5 + v1.heightspl * 0.5;
					newpoint.heightspl.x = newpoint.heightspl.y;
					newpoint.heightspl.y = getHeightAndRadius(newpoint.pos);
					newpoint.gridxy = Vector2Type(level.offs.x + x, level.offs.y + y) * _invNormalsPerVert;

					//addNoiseAndRadius(newpoint);
					if(x % _normalsPerVert == 0 && y % _normalsPerVert == 0)
					{
						// correct height to match parents height taking into account normals per vertex
						idx_type offsxm1d = calcDrawVertexOffset(offsym1d, offsxd);
						idx_type offsxp1d = calcDrawVertexOffset(offsyp1d, offsxd);
						SGCVertexDef& v0d = getDrawVertex(level, offsxm1d);
						SGCVertexDef& v1d = getDrawVertex(level, offsxp1d);
						newpoint.heightspl.x = v0d.heightspl.y * 0.5 + v1d.heightspl.y * 0.5;
						setDrawVertex(level, calcDrawVertexOffset(offsxd, offsyd), newpoint);
					}
					setVertex(level, offs, newpoint);
				}
			}

			// now fill the odd y, odd x gaps
			for(grid_index_type y = starty+1; y < endy-1; y+=2)
			{
				grid_index_type offsy = offsetY(level, y);
				grid_index_type offsyd = yToDrawOffset(level, y);
				grid_index_type offsym1 = offsetY(level, y-1);
				grid_index_type offsyp1 = offsetY(level, y+1);
				grid_index_type offsym1d = yToDrawOffset(level, y - _normalsPerVert);
				grid_index_type offsyp1d = yToDrawOffset(level, y + _normalsPerVert);
				for(grid_index_type x = startx+1; x < endx-1; x+=2)
				{
					grid_index_type offsxd = xToDrawOffset(level, x);
					idx_type offs = calcVertexOffset(offsetX(level, x), offsy);
					idx_type offsxm1 = calcVertexOffset(offsetX(level, x-1), offsym1);
					idx_type offsxp1 = calcVertexOffset(offsetX(level, x+1), offsyp1);

					//SGCVertexDef newpoint = ((getVertex(level, offsxm1) + getVertex(level, offsxp1)) * 0.5);
					//newpoint.pos = newpoint.pos.normal();
					//newpoint.heightspl.x = newpoint.heightspl.y;
					//newpoint.heightspl.y = getHeightAndRadius(newpoint.pos);
					//newpoint.gridxy = Vector2Type(level.offs.x + x, level.offs.y + y);

					SGCVertexDef& v0 = getVertex(level, offsxm1);
					SGCVertexDef& v1 = getVertex(level, offsxp1);
					SGCVertexDef newpoint;
					//	newpoint = (( + getVertex(level, offsxp1)) * 0.5);
					newpoint.pos = (v0.pos * factor + v1.pos * factor).normal();//newpoint.pos.normal();
					newpoint.heightspl = v0.heightspl * 0.5 + v1.heightspl * 0.5;
					newpoint.heightspl.x = newpoint.heightspl.y;
					newpoint.heightspl.y = getHeightAndRadius(newpoint.pos);
					newpoint.gridxy = Vector2Type(level.offs.x + x, level.offs.y + y) * _invNormalsPerVert;

					//addNoiseAndRadius(newpoint);
					if(x % _normalsPerVert == 0 && y % _normalsPerVert == 0)
					{
						// correct height to match parents height taking into account normals per vertex
						idx_type offsxm1d = calcDrawVertexOffset(offsym1d, xToDrawOffset(level, x+_normalsPerVert));
						idx_type offsxp1d = calcDrawVertexOffset(offsyp1d, xToDrawOffset(level, x-_normalsPerVert));
						SGCVertexDef& v0d = getDrawVertex(level, offsxm1d);
						SGCVertexDef& v1d = getDrawVertex(level, offsxp1d);
						newpoint.heightspl.x = v0d.heightspl.y * 0.5 + v1d.heightspl.y * 0.5;
						setDrawVertex(level, calcDrawVertexOffset(offsxd, offsyd), newpoint);
					}
					setVertex(level, offs, newpoint);
				}
			}
		}
		//updateTexture(level, 0, 0, _levelSize, _levelSize, 0, 0);
		//updateTexture(level, startx, starty, endx, endy);
		//level.verts->dirty(0, level.verts->count()-1);
	}

	coord_float_type getHeightAndRadius(const Vector3Type& point) const
	{
		return _heightData->get(point)*100 + _radius;
	}

	void addRadius(Vector3Type& point) const
	{
		point *= _radius;
	}

	void addNoiseAndRadius(Vector3Type& point) const
	{
		point *= getHeightAndRadius(point);
	}

	void setVertex(SGCLevelType& level, typename SGCLevelType::index_type offs, const SGCVertexDef& newpoint)
	{
		//SGCVertexDef* v = _vertexSpec.extract< SGCVertexDef >(*level.verts.get(), offs);
		//*v = newpoint;
		level.vertices[offs] = newpoint;
		//v->heightspl = heightspl;
		//v->gridxy = gridxy;
	}

	void setDrawVertex(SGCLevelType& level, typename SGCLevelType::index_type offs, const SGCVertexDef& newpoint)
	{
		SGCVertexDef* v = _vertexSpec.extract< SGCVertexDef >(*level.verts.get(), offs);
		*v = newpoint;
	}

	SGCVertexDef& getVertex(SGCLevelType& level, typename SGCLevelType::index_type offs)
	{
		//SGCVertexDef* v = _vertexSpec.extract< SGCVertexDef >(*level.verts.get(), offs);
		return level.vertices[offs];//*v;
	}

	const SGCVertexDef& getVertex(const SGCLevelType& level, typename SGCLevelType::index_type offs) const
	{
		//SGCVertexDef* v = _vertexSpec.extract< SGCVertexDef >(*level.verts.get(), offs);
		return level.vertices[offs];//*v;
	}

	SGCVertexDef& getDrawVertex(SGCLevelType& level, typename SGCLevelType::index_type offs)
	{
		SGCVertexDef* v = _vertexSpec.extract< SGCVertexDef >(*level.verts.get(), offs);
		return *v;
	}

	const SGCVertexDef& getDrawVertex(const SGCLevelType& level, typename SGCLevelType::index_type offs) const
	{
		SGCVertexDef* v = _vertexSpec.extract< SGCVertexDef >(*level.verts.get(), offs);
		return *v;
	}

	template < class VectorFloatType >
	void calculateHVVectors(const math::Vector3<VectorFloatType>& center)
	{
		_verticalPlaneNormal = Vector3Type::YAxis.crossp(center).normal();
		if(_verticalPlaneNormal.lengthSquared() == 0)
		{
			_verticalPlaneNormal = Vector3Type::XAxis.crossp(center).normal();
		}
		_horizontalPlaneNormal = center.crossp(_verticalPlaneNormal).normal();
	}

	template < class VectorFloatType >
	void recalculateHVVectors(const math::Vector3<VectorFloatType>& center)
	{
		_horizontalPlaneNormal = center.crossp(_verticalPlaneNormal).normal();
		_verticalPlaneNormal = _horizontalPlaneNormal.crossp(center).normal();
	}

	// initializes a level around the coordinate provided in center
	void createLevel(SGCLevelType& level, Vector3Type center)
	{
		// if we are creating base level
		if(level.n == 0)
		{
			// the center will be exactly the camera center
			level.center = center;
			// offset will be 0
			level.offs.x = level.offs.y = 0;
			//level.gridCameraCenter = calculateError(level, center);
		}
		// if we are creating a sub level
		else
		{
			// center new level in its parent
			// center is copied from parent
			level.center = _levels[level.n-1].center;
			// offset is calculated from parents offset + offset from parent (level size / 2)
			level.offs.x = static_cast<grid_index_type>(_levels[level.n-1].offs.x * 2 + (_levelSize - 1) * 0.5);
			level.offs.y = static_cast<grid_index_type>(_levels[level.n-1].offs.y * 2 + (_levelSize - 1) * 0.5);
			// then calculate the offset to its desired position closest to the camera
			calculateGridError(level, center);
			// update the offset
			level.offs += level.error;
			level.error.x = level.error.y = 0;
			// set the static offset (for aligning the texture) as the level offset
			level.staticOffset = level.offs; 
			
		}
		// the grid camera center is the continuous error from the grid center
		level.gridCameraCenter = calculateContinuousError(level, center);

		// initialize sectors
		level.sectors.resize(_sectorsPerLevelSide * _sectorsPerLevelSide + 1);
		SGCLevelType::index_type segmentsPerSectorSize = (_levelSize*0.5) / _sectorsPerLevelSide;
		SGCLevelType::SectorIterator sItr = level.sectors.begin();
		// initialize the sectors
		for(SGCLevelType::index_type x = 0, idx = 0;  x < _sectorsPerLevelSide; ++x)
		{
			for(SGCLevelType::index_type y = 0;  y < _sectorsPerLevelSide; ++y, ++sItr)
			{
				sItr->xoffs = x * segmentsPerSectorSize;
				sItr->yoffs = y * segmentsPerSectorSize;
				sItr->tris.reset(new SGCLevelType::TriangleSetType());
			}
		}

		// initialize the offset arrays
		level.xOffsets.clear();
		level.yOffsets.clear();
		for(grid_index_type gidx = 0; gidx < static_cast<grid_index_type>(_levelSize); ++gidx)
		{
			level.xOffsets.push_back(gidx);
			level.yOffsets.push_back(gidx);
		}

		// initialize level vertex data
		level.verts.reset(new SGCLevelType::VertexSetType());
		level.verts->resize(_drawVertsSize * _drawVertsSize, _vertexSpec.vertexSize());

		level.vertices.resize(_levelSize * _levelSize);
// 		for(unsigned int y = 0; y < _levelSize; ++y)
// 			vertices[y].resize(_levelSize, 0.0);

		// initialize the level material
		level.material.reset(new Material());
		std::stringstream ss;
		ss << "sgcmat" << level.n;
		level.material->setName(ss.str());
		// attach the shader
		level.material->setShader(_shader);
		// set the shader variables
		level.material->vector3fInputMap()["Ambient"] = math::Vector3f(0.1f, 0.1f, 0.1f);
		level.material->floatInputMap()["GridSize"] = /*_levelSize-1;*/_drawVertsSize-1;
		level.material->floatInputMap()["TransitionSize"] = static_cast<float>(_levelSize) * 0.06f;
		level.material->vector2fInputMap()["GridCameraCenter"] = level.gridCameraCenter * _invNormalsPerVert;
		level.material->vector2fInputMap()["GridOffset"] = level.offs * _invNormalsPerVert;
		level.material->vector3fInputMap()["Light0Dir"] = math::Vector3f(1.0f, 0.0f, 1.0f).normal();
		GLuint texsize = 1024;
		// unattach previous texture from fbo it was attached before it is deleted and recreated
		if(!level.texture.valid())
		{
			level.texture.create2D(GL_TEXTURE_2D, texsize, texsize, 3, GL_RGB, GL_UNSIGNED_BYTE, NULL, GL_RGB);
			unsigned int levelFboIdx = static_cast<unsigned int>(std::floor(level.n / static_cast<float>(FramebufferObject::GetMaxColorAttachments())));
			level.fbo = _fbos[levelFboIdx];
			level.fboAttachment = level.n % FramebufferObject::GetMaxColorAttachments();
			level.fbo->Bind();
			level.fbo->AttachTexture(level.texture.handle(), GL_COLOR_ATTACHMENT0 + level.fboAttachment);
			level.fbo->IsValid();
		}
		assert(level.texture.valid());
		if(!level.normalmap.valid())
		{
			level.normalmap.create2D(GL_TEXTURE_2D, _levelSize-1, _levelSize-1, 3, GL_RGB, GL_UNSIGNED_BYTE, NULL, GL_RGB);
		}

		// attach texture to material
		level.material->textureInputMap()["LevelTexture"] = level.texture.handle();
		level.material->textureInputMap()["LevelNormalMap"] = level.normalmap.handle();
		if(level.n != 0)
		{
			SGCLevelType& parent = _levels[level.n-1];
			level.material->vector2fInputMap()["ParentOffset"] = level.staticOffset - parent.offs * 2;//math::Vector2f(getParentX(level, parent, 0), getParentY(level, parent, 0));//(level.offs - parent.offs * 2) * 0.5;// * 0;//_invNormalsPerVert;
			level.material->textureInputMap()["ParentTexture"] = parent.texture.handle();
			level.material->textureInputMap()["ParentNormalMap"] = parent.normalmap.handle();
		}
		else
		{
			level.material->vector2fInputMap()["ParentOffset"] = math::Vector2f();// * 0;//_invNormalsPerVert;
			level.material->textureInputMap()["ParentTexture"] = level.texture.handle();
			level.material->textureInputMap()["ParentNormalMap"] = level.normalmap.handle();
		}

		setActiveRegion(level);
		// initialize the vertex positions for the level
		recalculateVerts(level, 0, 0, _levelSize, _levelSize);
		// initialize the level texture
		updateTexture(level, level.offs.x, level.offs.y, level.offs.x+_levelSize, level.offs.y+_levelSize);
		updateNormalMap(level, level.offs.x, level.offs.y, level.offs.x+_levelSize, level.offs.y+_levelSize);
		// update the center from the new center vertex
		updateCenter(level);
	}

	void updateNormalMap(SGCLevelType& level, grid_index_type startx, grid_index_type starty, grid_index_type endx, grid_index_type endy)
	{
		grid_index_type offtsx = mod_abs(startx, _levelSize-1), offtex = mod_abs((endx-1), _levelSize-1);
		grid_index_type offtsy = mod_abs(starty, _levelSize-1), offtey = mod_abs((endy-1), _levelSize-1);

		grid_index_type offtsx0=0, offtex0=0;
		grid_index_type offtsx1=0, offtex1=0;
		grid_index_type startx0=0, endx0=0;
		grid_index_type startx1=0, endx1=0;
		grid_index_type offtsy0=0, offtey0=0;
		grid_index_type offtsy1=0, offtey1=0;
		grid_index_type starty0=0, endy0=0;
		grid_index_type starty1=0, endy1=0;
		// if tex end is less that tex start then we are definitely crossing the uv boundary
		if(offtex < offtsx)
		{
			offtsx0 = offtsx;
			startx0 = startx;
			endx0 = (startx - mod_abs(startx, _levelSize-1)) + (_levelSize-1);
			offtsx1 = 0;//offtsx;
			startx1 = endx0;
			endx1 = endx;
			assert((endx0 - startx0 + endx1 - startx1) == (endx - startx)); // full range of two strips == full range
			assert(startx0 != endx0 && startx1 != endx1);  // neither strip is zero in size
			assert(startx0 != startx1 && endx0 != endx1); // strips start and end at different locations
			assert(offtsx0 != offtsx1); // textures start in different places
		}
		// if tex start is tex end and the range is greater than 0 then we must be drawing and entire row
		// and also it probably crosses the uv boundary
		else if(offtex == offtsx && endx - startx > 1)
		{	
			offtsx0 = offtsx;
			startx0 = startx;
			endx0 = (startx - mod_abs(startx, _levelSize-1)) + (_levelSize-1);
			offtsx1 = 0;
			startx1 = endx0;
			endx1 = endx - 1;
			assert((endx0 - startx0 + endx1 - startx1) == (endx - startx - 1)); // full range of two strips == full range
		}
		// other possibilities are:
		// a) end tex is greater than start tex,
		// b) end tex is equal to start tex but range is only 1
		else
		{
			offtsx0 = offtsx;
			startx0 = startx;
			endx0 = endx;
			assert((endx0 - startx0) <= (_levelSize - 1 - offtsx)); // remaining space from tex start to end of tex is less than the size required to draw
		}

		assert(!(startx0 == startx1 && endx0 == endx1));

		if(offtey < offtsy)
		{
			offtsy0 = offtsy;
			starty0 = starty;
			endy0 = (starty - mod_abs(starty, _levelSize-1)) + (_levelSize-1);
			offtsy1 = 0;
			starty1 = endy0;
			endy1 = endy;
		}
		else if(offtey == offtsy && endy - starty > 1)
		{
			offtsy0 = offtsy;
			starty0 = starty;
			endy0 = (starty - mod_abs(starty, _levelSize-1)) + (_levelSize-1);
			offtsy1 = 0;
			starty1 = endy0;
			endy1 = endy - 1;
		}
		else
		{
			offtsy0 = offtsy;
			starty0 = starty;
			endy0 = endy;
		}

		level.normalmap.bind();
		drawNormalMap(level, startx0, starty0, endx0, endy0, offtsx0, offtsy0);
		drawNormalMap(level, startx1, starty0, endx1, endy0, offtsx1, offtsy0);
		drawNormalMap(level, startx0, starty1, endx0, endy1, offtsx0, offtsy1);
		drawNormalMap(level, startx1, starty1, endx1, endy1, offtsx1, offtsy1);
		level.normalmap.unbind();

		//for(grid_index_type y = starty0, yt = offtsy0; y <= endy0; ++y, ++yt)
		//{
		//	grid_index_type offy = mod_abs(y + yoffset, _levelSize);//, offy1 = mod_abs(y + 1 + yoffset, _levelSize);
		//	grid_index_type offyp1 = mod_abs(y + yoffset + 1, _levelSize);
		//	grid_index_type offym1 = mod_abs(y + yoffset - 1, _levelSize);
		//	glBegin(GL_TRIANGLE_STRIP);
		//	for(grid_index_type x = startx1, xt = offtsx1; x <= endx1; ++x, ++xt)
		//	{
		//		grid_index_type offx = mod_abs(x + xoffset, _levelSize);
		//		grid_index_type offxp1 = mod_abs(x + xoffset, _levelSize);
		//		grid_index_type offxm1 = mod_abs(x - xoffset, _levelSize);
		//		drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
		//		drawTextureVertex(u, v + splatScale, textureUsage[offy1][offx][0], textureUsage[offy1][offx][1], xt, yt+1);
		//	}
		//	glEnd();
		//}	
		//for(grid_index_type y = starty1, yt = offtsy1; y <= endy1; ++y, ++yt)
		//{
		//	float v = y * splatScale;
		//	grid_index_type offy = mod_abs(y + yoffset, _levelSize);//, offy1 = mod_abs(y + 1 + yoffset, _levelSize);
		//	glBegin(GL_TRIANGLE_STRIP);
		//	for(grid_index_type x = startx0, xt = offtsx0; x <= endx0; ++x, ++xt)
		//	{
		//		float u = x * splatScale;
		//		grid_index_type offx = mod_abs(x + xoffset, _levelSize);
		//		drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
		//		drawTextureVertex(u, v + splatScale, textureUsage[offy1][offx][0], textureUsage[offy1][offx][1], xt, yt+1);
		//	}
		//	glEnd();
		//}
		//for(grid_index_type y = starty1, yt = offtsy1; y <= endy1; ++y, ++yt)
		//{
		//	float v = y * splatScale;
		//	grid_index_type offy = mod_abs(y + yoffset, _levelSize);//, offy1 = mod_abs(y + 1 + yoffset, _levelSize);
		//	glBegin(GL_TRIANGLE_STRIP);
		//	for(grid_index_type x = startx1, xt = offtsx1; x <= endx1; ++x, ++xt)
		//	{
		//		float u = x * splatScale;
		//		grid_index_type offx = mod_abs(x + xoffset, _levelSize);
		//		drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
		//		drawTextureVertex(u, v + splatScale, textureUsage[offy1][offx][0], textureUsage[offy1][offx][1], xt, yt+1);
		//	}
		//	glEnd();
		//}

	}

	void drawNormalMap( SGCLevelType &level, grid_index_type startx0, grid_index_type starty0, grid_index_type endx0, grid_index_type endy0, grid_index_type offtsx0, grid_index_type offtsy0 )
	{
		grid_index_type xoffset = -level.staticOffset.x, yoffset = -level.staticOffset.y;
		std::vector< unsigned char > data;//(_levelSize * _levelSize * 3);
		for(grid_index_type y = starty0, yt = offtsy0; y < endy0; ++y, ++yt)
		{
			grid_index_type offy = mod_abs(y + yoffset, _levelSize);//, offy1 = mod_abs(y + 1 + yoffset, _levelSize);
			grid_index_type offyp1 = mod_abs(y + yoffset + 1, _levelSize);
			//grid_index_type offym1 = mod_abs(y + yoffset - 1, _levelSize);
			for(grid_index_type x = startx0, xt = offtsx0; x < endx0; ++x, ++xt)
			{
				grid_index_type offx = mod_abs(x + xoffset, _levelSize);
				grid_index_type offxp1 = mod_abs(x + xoffset + 1, _levelSize);
				//grid_index_type offxm1 = mod_abs(x + xoffset - 1, _levelSize);
				SGCVertexDef& tl = getVertex(level, calcVertexOffset(offx, offy));
				SGCVertexDef& tr = getVertex(level, calcVertexOffset(offxp1, offy));
				//SGCVertexDef& downVert = getVertex(level, calcVertexOffset(offx, offyp1));
				SGCVertexDef& bl = getVertex(level, calcVertexOffset(offx, offyp1));
				SGCVertexDef& br = getVertex(level, calcVertexOffset(offxp1, offyp1));

				Vector3Type tlv = tl.pos * tl.heightspl.y;
				Vector3Type trv = tr.pos * tr.heightspl.y;
				Vector3Type blv = bl.pos * bl.heightspl.y;
				Vector3Type brv = br.pos * br.heightspl.y;
				Vector3Type l = trv - tlv;
				Vector3Type d = blv - tlv;
				Vector3Type normal = d.crossp(l).normal();

				//Vector3Type centerOffs = centerVert.pos * centerVert.heightspl.y;
				//Vector3Type upV = upVert.pos * upVert.heightspl.y - centerOffs;
				//Vector3Type downV = downVert.pos * downVert.heightspl.y - centerOffs;
				//Vector3Type leftV = leftVert.pos * leftVert.heightspl.y - centerOffs;
				//Vector3Type rightV = rightVert.pos * rightVert.heightspl.y - centerOffs;
				//Vector3Type normal = (upV.crossp(rightV) + rightV.crossp(downV) + downV.crossp(leftV) + leftV.crossp(upV)).normal();
				//data[calcVertexOffset(xt, yt) * 3] = MY_MAX(0, normal.x * 128 + 127);
				//data[calcVertexOffset(xt, yt) * 3+1] = MY_MAX(0, normal.y * 128 + 127);
				//data[calcVertexOffset(xt, yt) * 3+2] = MY_MAX(0, normal.z * 128 + 127);
				data.push_back(MY_MAX(0, normal.x * 128 + 127));
				data.push_back(MY_MAX(0, normal.y * 128 + 127));
				data.push_back(MY_MAX(0, normal.z * 128 + 127));
			}
		}
		if(data.size() > 0)
			glTexSubImage2D(GL_TEXTURE_2D, 0, offtsx0, offtsy0, endx0 - startx0, endy0 - starty0, GL_RGB, GL_UNSIGNED_BYTE, &data[0]);
	}

	// coordinates must be the range of vertices corresponding to the texture area that requires updating, and they most include global level offset
	void updateTexture(SGCLevelType& level, grid_index_type startx, grid_index_type starty, grid_index_type endx, grid_index_type endy, bool preferXStrips = true)
	{
		assert(endx - startx <= _levelSize && endy - starty <= _levelSize);

		// 		startx = MY_MAX(startx, level.activeRegion.left);
		// 		starty = MY_MAX(starty, level.activeRegion.bottom);
		// 		endx = MY_MIN(endx, level.activeRegion.right);
		// 		endy = MY_MIN(endy, level.activeRegion.top);

		grid_index_type w = endx - startx, h = endy - starty;

		level.fbo->Bind();
		_fboBound = true;

		glDrawBuffer(GL_COLOR_ATTACHMENT0_EXT + level.fboAttachment);
		glViewport(0, 0, level.texture.width(), level.texture.height());
		glMatrixMode(GL_PROJECTION);
		glLoadIdentity();
		glOrtho(0, _levelSize-1, 0, _levelSize-1, -1, 1);

		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();

		_textureMaterial->bind();
		_textureShader->bind();
		std::vector < std::vector < std::vector < float > > > 
			textureUsage(_levelSize, std::vector< std::vector < float > > (_levelSize, std::vector < float >(2, 0.0f))); 
		calcTextureUsage(0, _levelSize, 0, _levelSize, level, textureUsage);
		while(_textureShader->startPass())
		{
			// calculate the vertex and texture coordinates
			grid_index_type offtsx = mod_abs(startx, _levelSize-1), offtex = mod_abs((endx-1), _levelSize-1);
			grid_index_type offtsy = mod_abs(starty, _levelSize-1), offtey = mod_abs((endy-1), _levelSize-1);

			grid_index_type offtsx0=0, offtex0=0;
			grid_index_type offtsx1=0, offtex1=0;
			grid_index_type startx0=0, endx0=0;
			grid_index_type startx1=0, endx1=0;
			grid_index_type offtsy0=0, offtey0=0;
			grid_index_type offtsy1=0, offtey1=0;
			grid_index_type starty0=0, endy0=0;
			grid_index_type starty1=0, endy1=0;

			// if tex end is less that tex start then we are definitely crossing the uv boundary
			if(offtex < offtsx)
			{
				offtsx0 = offtsx;
				startx0 = startx;
				endx0 = (startx - mod_abs(startx, _levelSize-1)) + (_levelSize-1);
				offtsx1 = 0;//offtsx;
				startx1 = endx0;
				endx1 = endx;
				assert((endx0 - startx0 + endx1 - startx1) == (endx - startx)); // full range of two strips == full range
				assert(startx0 != endx0 && startx1 != endx1);  // neither strip is zero in size
				assert(startx0 != startx1 && endx0 != endx1); // strips start and end at different locations
				assert(offtsx0 != offtsx1); // textures start in different places
			}
			// if tex start is tex end and the range is greater than 0 then we must be drawing and entire row
			// and also it probably crosses the uv boundary
			else if(offtex == offtsx && endx - startx > 1)
			{	
				offtsx0 = offtsx;
				startx0 = startx;
				endx0 = (startx - mod_abs(startx, _levelSize-1)) + (_levelSize-1);
				offtsx1 = 0;
				startx1 = endx0;
				endx1 = endx - 1;
				assert((endx0 - startx0 + endx1 - startx1) == (endx - startx - 1)); // full range of two strips == full range
			}
			// other possibilities are:
			// a) end tex is greater than start tex,
			// b) end tex is equal to start tex but range is only 1
			else
			{
				offtsx0 = offtsx;
				startx0 = startx;
				endx0 = endx;
				assert((endx0 - startx0) <= (_levelSize - 1 - offtsx)); // remaining space from tex start to end of tex is less than the size required to draw
			}

			assert(!(startx0 == startx1 && endx0 == endx1));

			if(offtey < offtsy)
			{
				offtsy0 = offtsy;
				starty0 = starty;
				endy0 = (starty - mod_abs(starty, _levelSize-1)) + (_levelSize-1);
				offtsy1 = 0;
				starty1 = endy0;
				endy1 = endy;
			}
			else if(offtey == offtsy && endy - starty > 1)
			{
				offtsy0 = offtsy;
				starty0 = starty;
				endy0 = (starty - mod_abs(starty, _levelSize-1)) + (_levelSize-1);
				offtsy1 = 0;
				starty1 = endy0;
				endy1 = endy - 1;
			}
			else
			{
				offtsy0 = offtsy;
				starty0 = starty;
				endy0 = endy;
			}

			float splatScale = 100.0f / pow(2.0f, static_cast<float>(level.n));

			if(preferXStrips)
			{
				grid_index_type xoffset = -level.staticOffset.x, yoffset = -level.staticOffset.y;
				for(grid_index_type y = starty0, yt = offtsy0; y < endy0; ++y, ++yt)
				{
					float v = y * splatScale;
					grid_index_type offy = mod_abs(y + yoffset, _levelSize), offy1 = mod_abs(y + 1 + yoffset, _levelSize);
					glBegin(GL_TRIANGLE_STRIP);
					for(grid_index_type x = startx0, xt = offtsx0; x <= endx0; ++x, ++xt)
					{
						float u = x * splatScale;
						grid_index_type offx = mod_abs(x + xoffset, _levelSize);
						drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
						drawTextureVertex(u, v + splatScale, textureUsage[offy1][offx][0], textureUsage[offy1][offx][1], xt, yt+1);
					}
					glEnd();
					glBegin(GL_TRIANGLE_STRIP);
					for(grid_index_type x = startx1, xt = offtsx1; x <= endx1; ++x, ++xt)
					{
						float u = x * splatScale;
						grid_index_type offx = mod_abs(x + xoffset, _levelSize);
						drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
						drawTextureVertex(u, v + splatScale, textureUsage[offy1][offx][0], textureUsage[offy1][offx][1], xt, yt+1);
					}
					glEnd();
				}	
				for(grid_index_type y = starty1, yt = offtsy1; y < endy1; ++y, ++yt)
				{
					float v = y * splatScale;
					grid_index_type offy = mod_abs(y + yoffset, _levelSize), offy1 = mod_abs(y + 1 + yoffset, _levelSize);
					glBegin(GL_TRIANGLE_STRIP);
					for(grid_index_type x = startx0, xt = offtsx0; x <= endx0; ++x, ++xt)
					{
						float u = x * splatScale;
						grid_index_type offx = mod_abs(x + xoffset, _levelSize);
						drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
						drawTextureVertex(u, v + splatScale, textureUsage[offy1][offx][0], textureUsage[offy1][offx][1], xt, yt+1);
					}
					glEnd();
					glBegin(GL_TRIANGLE_STRIP);
					for(grid_index_type x = startx1, xt = offtsx1; x <= endx1; ++x, ++xt)
					{
						float u = x * splatScale;
						grid_index_type offx = mod_abs(x + xoffset, _levelSize);
						drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
						drawTextureVertex(u, v + splatScale, textureUsage[offy1][offx][0], textureUsage[offy1][offx][1], xt, yt+1);
					}
					glEnd();
				}	
			}
			else
			{
				grid_index_type xoffset = -level.staticOffset.x, yoffset = -level.staticOffset.y;
				for(grid_index_type x = startx0, xt = offtsx0; x < endx0; ++x, ++xt)
				{
					float u = x * splatScale;
					grid_index_type offx = mod_abs(x + xoffset, _levelSize), offx1 = mod_abs(x + 1 + xoffset, _levelSize);
					glBegin(GL_TRIANGLE_STRIP);
					for(grid_index_type y = starty0, yt = offtsy0; y <= endy0; ++y, ++yt)
					{
						grid_index_type offy = mod_abs(y + yoffset, _levelSize);
						float v = y * splatScale;
						drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
						drawTextureVertex(u + splatScale, v, textureUsage[offy][offx1][0], textureUsage[offy][offx1][1], xt+1, yt);
					}
					glEnd();
					glBegin(GL_TRIANGLE_STRIP);
					for(grid_index_type y = starty1, yt = offtsy1; y <= endy1; ++y, ++yt)
					{
						float v = y * splatScale;
						grid_index_type offy = mod_abs(y + yoffset, _levelSize);
						drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
						drawTextureVertex(u + splatScale, v, textureUsage[offy][offx1][0], textureUsage[offy][offx1][1], xt+1, yt);
					}
					glEnd();
				}	
				for(grid_index_type x = startx1, xt = offtsx1; x < endx1; ++x, ++xt)
				{
					float u = x * splatScale;
					grid_index_type offx = mod_abs(x + xoffset, _levelSize), offx1 = mod_abs(x + 1 + xoffset, _levelSize);
					glBegin(GL_TRIANGLE_STRIP);
					for(grid_index_type y = starty0, yt = offtsy0; y <= endy0; ++y, ++yt)
					{
						grid_index_type offy = mod_abs(y + yoffset, _levelSize);
						float v = y * splatScale;
						drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
						drawTextureVertex(u + splatScale, v, textureUsage[offy][offx1][0], textureUsage[offy][offx1][1], xt+1, yt);
					}
					glEnd();
					glBegin(GL_TRIANGLE_STRIP);
					for(grid_index_type y = starty1, yt = offtsy1; y <= endy1; ++y, ++yt)
					{
						float v = y * splatScale;
						grid_index_type offy = mod_abs(y + yoffset, _levelSize);
						drawTextureVertex(u, v, textureUsage[offy][offx][0], textureUsage[offy][offx][1], xt, yt);
						drawTextureVertex(u + splatScale, v, textureUsage[offy][offx1][0], textureUsage[offy][offx1][1], xt+1, yt);
					}
					glEnd();
				}	
			}
		}
	}

	inline void drawTextureVertex( float u, float v, float w0, float w1, grid_index_type xt, grid_index_type yt )
	{
		glMultiTexCoord2f(GL_TEXTURE0, u, v);
		glMultiTexCoord4f(GL_TEXTURE1, w0, w1, 0.0f, 0.0f);
		glVertex2i(xt, yt);
	}

	void calcTextureUsage( grid_index_type starty, grid_index_type endy, grid_index_type startx, grid_index_type endx, SGCLevelType& level, std::vector< std::vector < std::vector < float > > >& textureUsage )
	{
		for(grid_index_type y = starty, yt = 0; y < endy; ++y, ++yt)
		{
			for(grid_index_type x = startx, xt = 0; x < endx; ++x, ++xt)
			{
				grid_index_type xoffs = offsetX(level, x);//(x+level.offs.x)%(_levelSize-1);
				//if(xoffs < 0)
				//	xoffs = - xoffs;
				grid_index_type yoffs = offsetY(level, y);//(y+level.offs.y)%(_levelSize-1);
				//if(yoffs < 0)
				//	yoffs = - yoffs;
				SGCVertexDef& v = getVertex(level, calcVertexOffset(xoffs, yoffs));
				textureUsage[yoffs][xoffs][0] = (v.heightspl.y - _radius) < 0? 0.0f : 1.0f;
				textureUsage[yoffs][xoffs][1] = (v.heightspl.y - _radius) < 0? 1.0f : 0.0f;
			}
		}
	}  
	void createSectors(SGCLevelType& level, grid_index_type startx, grid_index_type starty, grid_index_type endx, grid_index_type endy)
	{
		const SGCLevelType* child = NULL;
		const math::Rectangle<grid_index_type> levelRect(0, 0, _levelSize, _levelSize);

		if(level.n == _currLevels-1)
			child = NULL;
		else
			child = &_levels[level.n+1];

		SGCLevelType::SectorIterator sItr = level.sectors.begin();
		for(grid_index_type x = 0; x < _sectorsPerLevelSide; ++x)
		{
			for(grid_index_type y = 0; y < _sectorsPerLevelSide; ++y, ++sItr)
			{
				sItr->tris.reset(new SGCLevelType::TriangleSetType(TrianglePrimitiveType::TRIANGLE_STRIP, level.verts));
				sItr->tris->setMaterial(level.material);

				using namespace std;
				grid_index_type startx = MY_MAX(x * _sectorSize, level.activeRegion.left); 
				grid_index_type starty = MY_MAX(y * _sectorSize, level.activeRegion.bottom);
				grid_index_type endx = MY_MIN((x+1) * _sectorSize/* + _normalsPerVert*/, level.activeRegion.right);
				grid_index_type endy = MY_MIN((y+1) * _sectorSize/* + _normalsPerVert*/, level.activeRegion.top);

				if(startx < endx && starty < endy)
				{
					bool cisect = false;

					grid_index_type pstartx, pendx, pstarty, pendy;
					if(child != NULL)
					{
						grid_index_type cstartx = getChildX(level, *child, startx);
						grid_index_type cstarty = getChildY(level, *child, starty);
						grid_index_type cendx = getChildX(level, *child, endx);
						grid_index_type cendy = getChildY(level, *child, endy);
						math::Rectangle<grid_index_type> crect(cstartx, cstarty, cendx, cendy);
						cisect = math::intersects(crect, levelRect).occured;

						pstartx = getParentX(*child, level, 0);
						pendx = getParentX(*child, level, _levelSize-1);
						pstarty = getParentY(*child, level, 0);
						pendy = getParentY(*child, level, _levelSize-1);
					}


					// if this sector covers part of the child area then we need to check for clipping
					if(cisect)
					{
						bool clear = false, hanging = false;
						bool lastidxisect = true, lastidx2isect = true;

						index_type lastusedidx = 0;
						for(grid_index_type y = starty; y < endy/*-_normalsPerVert*/; y+=_normalsPerVert)
						{
							index_type lastidx = 0;

							grid_index_type clippedstartx=startx, clippedendx=endx;
							if(y >= pstarty && y < pendy)
							{
								if(startx < pstartx)
								{
									clippedstartx = startx;
									clippedendx = MY_MIN(pstartx+1, endx);
								}
								else
								{
									clippedstartx = MY_MAX(pendx, startx);
									clippedendx = MY_MAX(pendx, endx);
								}
							}

							grid_index_type offsy = yToDrawOffset(level, y);//mod_abs<grid_index_type>(dy, _drawVertsSize);//offsetY(level, y);
							grid_index_type offsy1 = yToDrawOffset(level, y+_normalsPerVert); //mod_abs<grid_index_type>(dy + 1, _drawVertsSize);//offsetY(level, y + _normalsPerVert);

							for(grid_index_type x = clippedstartx; x <= clippedendx; x+=_normalsPerVert)
							{
								grid_index_type offsx = xToDrawOffset(level, x);//mod_abs<grid_index_type>(dx, _drawVertsSize);//offsetX(level, x);
								// if we are an even number 
								index_type idx = calcDrawVertexOffset(offsx, offsy);//offsy * _drawVertsSize + offsx;//calcDrawVertexOffset(offsx, offsy);//offsetY(level, y) * _invNormalsPerVert * _drawVertsSize +  offsetX(level, x)*_invNormalsPerVert;////calcBufferOffset(offsetX(level, x)*_invNormalsPerVert, offsetY(level, y)*_invNormalsPerVert);
								index_type idx2 = calcDrawVertexOffset(offsx, offsy1); //offsy1 * _drawVertsSize + offsx;//calcDrawVertexOffset(offsx, offsy1);//offsetY(level, y+_normalsPerVert) * _invNormalsPerVert * _drawVertsSize +  offsetX(level, x)*_invNormalsPerVert;//calcBufferOffset(offsetX(level, x)*_invNormalsPerVert, offsetY(level, y+2)*_invNormalsPerVert);

								//index_type idx = //calcVertexOffset(offsetX(level, x), offsetY(level, y));
								//index_type idx2 = //calcVertexOffset(offsetX(level, x), offsetY(level, y+_normalsPerVert));

								// finish degenerate tri at the start of the row
								if(x == clippedstartx && y != starty)
								{
									sItr->tris->push_back(idx);
								}

								sItr->tris->push_back(idx);
								sItr->tris->push_back(idx2);
								lastidx = idx2;
							}
							// start degenerate tri at the end of the row
							if(y != endy-_normalsPerVert)
								sItr->tris->push_back(lastidx);
						}
					}
					else
					{
						index_type lastidx;
						//grid_index_type dy = (starty + level.offs.y) * _invNormalsPerVert;
						
						for(grid_index_type y = starty; y < endy/*-1*/; y+=_normalsPerVert/*, ++dy*/)
						{
							grid_index_type offsy = yToDrawOffset(level, y);//mod_abs<grid_index_type>(dy, _drawVertsSize);//offsetY(level, y);
							grid_index_type offsy1 = yToDrawOffset(level, y+_normalsPerVert); //mod_abs<grid_index_type>(dy + 1, _drawVertsSize);//offsetY(level, y + _normalsPerVert);
							//grid_index_type dx = (startx + level.offs.x) * _invNormalsPerVert;
							for(grid_index_type x = startx; x <= endx; x+=_normalsPerVert/*, ++dx*/)
							{
								grid_index_type offsx = xToDrawOffset(level, x);//mod_abs<grid_index_type>(dx, _drawVertsSize);//offsetX(level, x);
								//assert(offsx % 2 == 0 && offsy % 2 == 0 && offsy1 % 2 == 0);
								// if we are an even number
								//assert(offsy % 2 == 0 && offsy1 % 2 == 0 && offsx % 2 == 0);
								index_type idx = calcDrawVertexOffset(offsx, offsy);//offsy * _drawVertsSize + offsx;//calcDrawVertexOffset(offsx, offsy);//offsetY(level, y) * _invNormalsPerVert * _drawVertsSize +  offsetX(level, x)*_invNormalsPerVert;////calcBufferOffset(offsetX(level, x)*_invNormalsPerVert, offsetY(level, y)*_invNormalsPerVert);
								index_type idx2 = calcDrawVertexOffset(offsx, offsy1); //offsy1 * _drawVertsSize + offsx;//calcDrawVertexOffset(offsx, offsy1);//offsetY(level, y+_normalsPerVert) * _invNormalsPerVert * _drawVertsSize +  offsetX(level, x)*_invNormalsPerVert;//calcBufferOffset(offsetX(level, x)*_invNormalsPerVert, offsetY(level, y+2)*_invNormalsPerVert);

								assert(idx < level.verts->count() && idx2 < level.verts->count());
								// finish degenerate tri at the start of the row
								if(x == startx && y != starty)
								{
									sItr->tris->push_back(idx);
								}

								sItr->tris->push_back(idx);
								sItr->tris->push_back(idx2);
								lastidx = idx2;
							}
							// start degenerate tri at the end of the row
							if(y != endy-_normalsPerVert)
								sItr->tris->push_back(lastidx);
						}
					}
				}
			}
		}

		sItr->tris.reset(new SGCLevelType::TriangleSetType(TrianglePrimitiveType::TRIANGLES, level.verts));
		sItr->tris->setMaterial(level.material);
		// create zero area triangles at edges
		grid_index_type yoffs = yToDrawOffset(level, 0);//offsetY(level, 0);
		for(grid_index_type x = 0; x < _levelSize-2*_normalsPerVert; x+=2*_normalsPerVert)
		{
			index_type idx0 = calcDrawVertexOffset(xToDrawOffset(level, x), yoffs);//calcBufferOffset(offsetX(level, x), yoffs);
			index_type idx1 = calcDrawVertexOffset(xToDrawOffset(level, x+1*_normalsPerVert), yoffs);//calcBufferOffset(offsetX(level, x+1*_normalsPerVert), yoffs);
			index_type idx2 = calcDrawVertexOffset(xToDrawOffset(level, x+2*_normalsPerVert), yoffs);//calcBufferOffset(offsetX(level, x+2*_normalsPerVert), yoffs);
			sItr->tris->push_back(idx0);
			sItr->tris->push_back(idx1);
			sItr->tris->push_back(idx2);
		}
		yoffs = yToDrawOffset(level, _levelSize-1);//offsetY(level, _levelSize-1);
		for(grid_index_type x = 0; x < _levelSize-2*_normalsPerVert; x+=2*_normalsPerVert)
		{
			index_type idx0 = calcDrawVertexOffset(xToDrawOffset(level, x), yoffs);//calcBufferOffset(offsetX(level, x), yoffs);
			index_type idx1 = calcDrawVertexOffset(xToDrawOffset(level, x+1*_normalsPerVert), yoffs);//calcBufferOffset(offsetX(level, x+1*_normalsPerVert), yoffs);
			index_type idx2 = calcDrawVertexOffset(xToDrawOffset(level, x+2*_normalsPerVert), yoffs);//calcBufferOffset(offsetX(level, x+2*_normalsPerVert), yoffs);
			sItr->tris->push_back(idx0);
			sItr->tris->push_back(idx1);
			sItr->tris->push_back(idx2);
		}

		grid_index_type xoffs = xToDrawOffset(level, 0);//offsetX(level, 0);
		for(grid_index_type x = 0; x < _levelSize-2*_normalsPerVert; x+=2*_normalsPerVert)
		{
			index_type idx0 = calcDrawVertexOffset(xoffs, yToDrawOffset(level, x));//calcBufferOffset(xoffs, offsetY(level, x));
			index_type idx1 = calcDrawVertexOffset(xoffs, yToDrawOffset(level, x+1*_normalsPerVert));//calcBufferOffset(xoffs, offsetY(level, x+1*_normalsPerVert));
			index_type idx2 = calcDrawVertexOffset(xoffs, yToDrawOffset(level, x+2*_normalsPerVert));//calcBufferOffset(xoffs, offsetY(level, x+2*_normalsPerVert));
			sItr->tris->push_back(idx0);
			sItr->tris->push_back(idx1);
			sItr->tris->push_back(idx2);
		}
		xoffs = xToDrawOffset(level, _levelSize-1);//offsetX(level, _levelSize-1);
		for(grid_index_type x = 0; x < _levelSize-2*_normalsPerVert; x+=2*_normalsPerVert)
		{
			index_type idx0 = calcDrawVertexOffset(xoffs, yToDrawOffset(level, x));//calcBufferOffset(xoffs, offsetY(level, x));
			index_type idx1 = calcDrawVertexOffset(xoffs, yToDrawOffset(level, x+1*_normalsPerVert));//calcBufferOffset(xoffs, offsetY(level, x+1*_normalsPerVert));
			index_type idx2 = calcDrawVertexOffset(xoffs, yToDrawOffset(level, x+2*_normalsPerVert));//calcBufferOffset(xoffs, offsetY(level, x+2*_normalsPerVert));
			sItr->tris->push_back(idx0);
			sItr->tris->push_back(idx1);
			sItr->tris->push_back(idx2);
		}
	}

	typename SGCLevelType::Sector& getSector(SGCLevelType& level, index_type xoffs, index_type yoffs)
	{
		return level.sectors[(xoffs*_sectorsPerLevelSide/_levelSize) + (yoffs*_sectorsPerLevelSide/_levelSize) * _sectorsPerLevelSide]
	}

	const typename SGCLevelType::Sector& getSector(const SGCLevelType& level, index_type xoffs, index_type yoffs) const
	{
		return level.sectors[(xoffs*_sectorsPerLevelSide/_levelSize) + (yoffs*_sectorsPerLevelSide/_levelSize) * _sectorsPerLevelSide]
	}

	void calculateGridError(SGCLevelType& level, const Vector3Type& newCenter)
	{
		
		Vector2Type error;

		// only do this for non base levels that have a grid with a center point
		if(level.n == 0)
		{
			error = getError(newCenter, level.center);
			//return Vector2Type(error.x/level.segmentAngle, -error.y/level.segmentAngle);
			level.error = (SGCLevelType::GridVectorType(error.x/level.segmentAngle, -error.y/level.segmentAngle) * _invNormalsPerVert) * _normalsPerVert;
		}
		else if(level.n == 1)
		{
			//error = getError(newCenter, level.center);
			//return error * 2;
			level.error = _levels[0].error * 2; //<============================== allows odd to become even
		}
		else
		{
			error = getError(newCenter, level.center);
			//return Vector2Type(error.x/_levels[level.n-1].segmentAngle, -error.y/_levels[level.n-1].segmentAngle) * 2;
			level.error = (SGCLevelType::GridVectorType(error.x/_levels[level.n-1].segmentAngle, -error.y/_levels[level.n-1].segmentAngle) * _invNormalsPerVert) * 2 * _normalsPerVert;
		}
	}

	Vector2Type calculateContinuousError(SGCLevelType& level, const Vector3Type& newCenter)
	{
		Vector2Type error = getError(newCenter, level.center);
		return Vector2Type(error.x/level.segmentAngle, -error.y/level.segmentAngle);
	}

	math::Vector2< coord_float_type > getError(const Vector3Type& center, const Vector3Type& from) const
	{
		return math::Vector2< coord_float_type >(getHorizontalRotation(from) - getHorizontalRotation(center),
			getVerticalRotation(from) - getVerticalRotation(center));
	}

	coord_float_type getHorizontalRotation(const Vector3Type& from) const
	{
		math::IntersectionPair< coord_float_type > isect = math::intersects(from, math::Plane< coord_float_type >(_horizontalPlaneNormal, Vector3Type::Zero));

		Vector3Type verticalIntersect = isect.point.normal();

		// now get angle between intersects and grid center
		coord_float_type verticalAngle = math::rad_to_deg(_verticalPlaneNormal.angle(verticalIntersect));
		verticalAngle -= 90.0f;

		return verticalAngle;
	}

	coord_float_type getVerticalRotation(const Vector3Type& from) const
	{
		math::IntersectionPair< coord_float_type > isect = math::intersects(from, math::Plane< coord_float_type >(_verticalPlaneNormal, Vector3Type::Zero));

		Vector3Type horizontalIntersect = isect.point.normal();

		// now get angle between intersects and grid center
		coord_float_type horizontalAngle = math::rad_to_deg(_horizontalPlaneNormal.angle(horizontalIntersect));
		horizontalAngle -= 90.0f;

		return horizontalAngle;
	}

	void setActiveRegion(SGCLevelType& level)
	{
// 		if(level.n > 0)
// 		{
// 			const SGCLevelType& parent = _levels[level.n-1];
// 			level.activeRegion.left = MY_MAX(0, getChildX(parent, level, parent.activeRegion.left));
// 			level.activeRegion.bottom = MY_MAX(0, getChildY(parent, level, parent.activeRegion.bottom));
// 			level.activeRegion.right = MY_MIN(_levelSize, getChildX(parent, level, parent.activeRegion.right));
// 			level.activeRegion.top = MY_MIN(_levelSize, getChildY(parent, level, parent.activeRegion.top));
// 		}
// 		else
		{
			level.activeRegion = SGCLevelType::RectangleType(0, 0, _levelSize, _levelSize);
		}
	}
};

}

#endif // _SCENE_SGCLIPMAP_HPP