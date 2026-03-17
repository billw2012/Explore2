
#define _USE_MATH_DEFINES
#include <cmath>

#include "glbase/sdlgl.hpp"

#include "planetgeometry.h"

#include "Utils/Profiler.h"

#include <vector>

#include <unordered_set>

namespace explore2 { ;

using namespace math;
using namespace scene::transform;

/*
Planets need to be renderable as impostors.
Only one planet is rendered at a time, only one in real-time, 
others are impostors, updated as needed.
The planet to render in real time is decided by:
*	distance - THIS ONE, simple, general
*	transversal rate - will fail when player is stationary, 
	even if they are in the atmosphere.
*	size on screen - does not relate to the rate at which it
	changes, tiny moon next to gas giant might appear 
	smaller but change more quickly relative to player.

Impostor planets do not need to process chunk update logic.
Only need to generate texture to the resolution required by 
their size on screen, this is also true for active planet, 
but high res texture takes long enough that player could 
get close to planet before it is finished being created.
Special logic required for texture update of impostor 
planets then, but keep it simple to start with.

Impostor planets do not need high res sphere, but as they
are rendered only rarely the cost should be amortized.

Need impostor manager to generalize handling, updating 
and creation of impostors. Impostor class interface?
Generalized renderable game object class interface, with 
impostor functionality, for planets, ships, suns, etc.?
Game update loop updates a list of objects, passing in 
game state, objects can add and remove renderable geometry
themselves.

Atmosphere rendering and planets:
Only one atmosphere can be rendered at a time. 
Atmosphere rendering has two parts: planet lighting, using 
precalculated textures, and atmosphere rendering.
Light pass renders over multiple objects, possibly multiple 
planets. Impostor planets already have their lighting and
atmosphere rendered, so need to be flagged for no lighting.
All other objects can be lit using the solar lighting pass,
as any that are out of the atmosphere of the active planet
will be outside the range of the atmospheric effects.
Atmosphere pass can apply to all objects.
*/

PlanetGeometry::PlanetGeometry()
	: _vertexSpec(new glbase::VertexSpec()),
	//_sharedChunkData.vertexSpec(new scene::VertexSpec()),
	//_sharedChunkData.vertexWaterSpec(new scene::VertexSpec()),
	//_sharedChunkData.vertexWaterL0Spec(new scene::VertexSpec()),
	_initalTextureSize(16),
	_currentTextureSize(16),
	_targetTextureSize(1024),
	_built(false), 
	_building(false),
	_buildingTexture(false),
	_chunksBuilt(false),
	_creatingChunks(false),
	_state(State::Empty)
{
	using namespace scene;
	_vertexSpec->append(glbase::VertexData::PositionData, 0, sizeof(float), 3, glbase::VertexElementType::Float);
	_vertexSpec->append(glbase::VertexData::TexCoord0, 1, sizeof(float), 2, glbase::VertexElementType::Float);
	assert(sizeof(PlanetSurfaceVertex) == _vertexSpec->vertexSize());

	init_state();
}

void PlanetGeometry::init_adjacency()
{
	using namespace math;

	Matrix3i rot000(
		1,	0,	0, 
		0,	1,	0, 
		0,	0,	1);
	Matrix3i rot090(
		0,	-1,	int(_gridSize) - 2, 
		1,	0,	0, 
		0,	0,	1);
	Matrix3i rot180(
		-1,	0,	int(_gridSize) - 2, 
		0,	-1,	int(_gridSize) - 2, 
		0,	0,	1);
	Matrix3i rot270(
		0,	1,	0, 
		-1,	0,	int(_gridSize) - 2, 
		0,	0,	1);
	// front face
	_adjaceny[0].left	= AdjacencyInfo::Side(2, rot000);
	_adjaceny[0].right	= AdjacencyInfo::Side(3, rot000);
	_adjaceny[0].top	= AdjacencyInfo::Side(4, rot000);
	_adjaceny[0].bottom = AdjacencyInfo::Side(5, rot000);
	// back face
	_adjaceny[1].left	= AdjacencyInfo::Side(3, rot000);
	_adjaceny[1].right	= AdjacencyInfo::Side(2, rot000);
	_adjaceny[1].top	= AdjacencyInfo::Side(4, rot180);
	_adjaceny[1].bottom = AdjacencyInfo::Side(5, rot180);
	// left face
	_adjaceny[2].left	= AdjacencyInfo::Side(1, rot000);
	_adjaceny[2].right	= AdjacencyInfo::Side(0, rot000);
	_adjaceny[2].top	= AdjacencyInfo::Side(4, rot090);
	_adjaceny[2].bottom = AdjacencyInfo::Side(5, rot270);
	// right face
	_adjaceny[3].left	= AdjacencyInfo::Side(0, rot000);
	_adjaceny[3].right	= AdjacencyInfo::Side(1, rot000);
	_adjaceny[3].top	= AdjacencyInfo::Side(4, rot180);
	_adjaceny[3].bottom = AdjacencyInfo::Side(5, rot090);
	// top face
	_adjaceny[4].left	= AdjacencyInfo::Side(2, rot270);
	_adjaceny[4].right	= AdjacencyInfo::Side(3, rot090);
	_adjaceny[4].top	= AdjacencyInfo::Side(1, rot180);
	_adjaceny[4].bottom = AdjacencyInfo::Side(0, rot000);
	// bottom face
	_adjaceny[5].left	= AdjacencyInfo::Side(2, rot090);
	_adjaceny[5].right	= AdjacencyInfo::Side(3, rot270);
	_adjaceny[5].top	= AdjacencyInfo::Side(0, rot000);
	_adjaceny[5].bottom = AdjacencyInfo::Side(1, rot180);
}

struct PlanetTextureGround : public PlanetTexture
{
	PlanetTextureGround() : PlanetTexture()
	{
		glbase::Texture::ptr tex(new glbase::Texture());
		tex->load("../Data/PlanetTextures/Grass.png", glbase::LoadOptions::GenerateMipmaps);
		tex->generate_mipmaps();
		set_texture(tex);
	}

	virtual void get_texture_weights(const math::SSETraits<float>::Vector& heights, 
		const math::SSETraits<float>::Vector& polarAngles, 
		const math::SSETraits<float>::Vector& verticalAngles,
		std::vector<float>& weights) const
	{
		weights.resize(heights.size());
		for(size_t idx = 0; idx < heights.size(); ++idx)
		{
			weights[idx] = heights[idx] > 0.0f ? 1.0f : 0.0f;
		}
	}

	virtual PlanetTexture* clone() const
	{
		return new PlanetTextureGround();
	}
};

struct PlanetTextureWater : public PlanetTexture
{
	PlanetTextureWater() : _waterColour(0, 0, 255)
	{
		glbase::Texture::ptr tex(new glbase::Texture());
		tex->load("../Data/PlanetTextures/Plains.png", glbase::LoadOptions::GenerateMipmaps);
		tex->generate_mipmaps();
		set_texture(tex);
	}

	virtual void get_texture_weights(const math::SSETraits<float>::Vector& heights, 
		const math::SSETraits<float>::Vector& polarAngles, 
		const math::SSETraits<float>::Vector& verticalAngles,
		std::vector<float>& weights) const
	{
		weights.resize(heights.size());
		for(size_t idx = 0; idx < heights.size(); ++idx)
		{
			weights[idx] = heights[idx] < 0.0f ? 1.0f : 0.0f;
		}
	}

	virtual PlanetTexture* clone() const
	{
		return new PlanetTextureWater();
	}

	virtual const Colour3b& get_colour() const
	{
		return _waterColour;
	}
private:
	Colour3b _waterColour;
};	

void PlanetGeometry::init(scene::transform::Group::ptr parent, 
				  render::GeometrySet::ptr geometry, 
				  render::GeometrySet::ptr distantGeometry,
				  AtmosphereParameters::ptr atmosphere,
				  float radius, bool water, float waterRadius,
				  int gridSize/* = 17*/, 
				  int initalTextureSize/* = 8*/, 
				  int targetTextureSize/* = 1024*/, 
				  int chunkTextureSize/* = 256*/,
				  float heightScaleFactor/* = 0.01f*/,
				  unsigned short seed /*= 0*/)
{
	using namespace scene;

	assert(parent);
	assert(geometry);
	assert(gridSize % 2 == 1);

	_atmosphere = atmosphere;

	_heightScaleFactor = heightScaleFactor;

	_sharedChunkData.heightData.reset(new HeightDataProvider(radius * heightScaleFactor, 1.0f, 14, seed)); // default height for now

	_sharedChunkData.textureData.reset(new PlanetTextureBlender());

	_sharedChunkData.texturingFBO.reset(new FramebufferObject());

	PlanetTexture::ptr groundTexture(new PlanetTextureGround());
	_sharedChunkData.textureData->add_texture(groundTexture);
	PlanetTexture::ptr waterTexture(new PlanetTextureWater());
	_sharedChunkData.textureData->add_texture(waterTexture);

	_sharedChunkData.geometry = geometry;

	_gridSize = gridSize;
	_sharedChunkData.radius = radius;
	_water = water;
	_sharedChunkData.waterRadius = waterRadius;
	_sharedChunkData.textureRes = chunkTextureSize;//(targetTextureSize * _quadsPerChunk) / (_gridSize-1);
	_sharedChunkData.gridSize = (int)(targetTextureSize / ((_gridSize-1) * 2) + 1);

	_sharedChunkData.maxResolution = 1.0f;
	// set activation distance for chunks to approx 5 times their size
	_chunkActivationDistance = (((2.0f * (float)M_PI * _sharedChunkData.radius) / 4.0f) / (float)_gridSize) * 10.0f;

	_built = false;
	_building = false;
	_buildingTexture = false;
	_chunksBuilt = false;
	_creatingChunks = false;

	_initalTextureSize = initalTextureSize;
	_currentTextureSize = 0;
	_targetTextureSize = targetTextureSize;

	_pixelAreaToCreate = 0.5;
	_pixelAreaToShow = 100;
	_pixelAreaToActivateChunks = 50000;

	_sharedChunkData.rootTransform.reset(new transform::Group());
	parent->insert(_sharedChunkData.rootTransform);

	// texGridSize is the number of vertices on a side of the splat grid. 
	// _sharedChunkData.gridSize holds the number of verts on a side in the geom grid, which is an ODD number
	// because we want an even number of quads, so odd number of verts: |q|q|q|q| 4 quads, 5 edges between them.
	// We want the texture splat to have twice the resolution, the same as the normal map, so 2 * gridSize - 1 gets us there.
	// e.g. gridSize = 5, for 4 geom grid quads, 5 * 2 - 1 = 9: |q|q|q|q|q|q|q|q|.
	int splatGridSize = _sharedChunkData.gridSize * 2 - 1;
	// create the material, vertex and index sets used to splat the chunk textures
	_sharedChunkData.textureCreationMaterial.reset(new scene::Material());
	_sharedChunkData.textureCreationMaterial->set_effect(_textureCreationShader);
	CHECK_OPENGL_ERRORS;

	transform::Camera camera;
	camera.set_type(transform::ProjectionType::Orthographic);
	// splatGridSize - 1 because 0 based..
	camera.set_render_area(math::Rectanglef(0, 0, static_cast<float>(splatGridSize - 1), static_cast<float>(splatGridSize - 1)));
	//Viewport::ptr viewport(new Viewport(0, 0, (unsigned int)_sharedChunkData.textureRes, (unsigned int)_sharedChunkData.textureRes));
	//camera.set_viewport(viewport);
	camera.set_near_plane(Camera::float_type(-1));
	camera.set_far_plane(Camera::float_type(1));

	_sharedChunkData.textureCreationMaterial->set_parameter("MODELVIEWPROJMAT", math::Matrix4f(camera.projection()));

	_sharedChunkData.textureCreationTris.reset(new glbase::TriangleSet(glbase::TrianglePrimitiveType::TRIANGLES));
	glbase::VertexSpec::ptr textureCreationVertexSpec(new glbase::VertexSpec());
	// position
	textureCreationVertexSpec->append(glbase::VertexData::PositionData, 0, sizeof(float), 2, glbase::VertexElementType::Float);
	// u, v, weight (alpha of the texture being splatted)
	textureCreationVertexSpec->append(glbase::VertexData::TexCoord0, 1, sizeof(float), 3, glbase::VertexElementType::Float);
	_sharedChunkData.textureCreationVerts.reset(new glbase::VertexSet(textureCreationVertexSpec, splatGridSize * splatGridSize, GL_STREAM_DRAW));
	// number of indices for tri strips with degenerate triangles:
	//   0 . . . x    0.....x            
	// 0 o-o-o-o-o 0  0--2--4    0--1--2
	// . |/|/|/|/| .  |/ |/ |    |/ |/ |
	// . o-o-o-o-o .  1--3--5,6  3--4--5
	// . |/|/|/|/| . 7,8-10-12   |/ |/ |
	// y o-o-o-o-o .  |/ |/ |    6--7--8
	// 			   y  9--11-13

	for(int y = 0; y < splatGridSize - 1; ++y)
	{
		for(int x = 0; x < splatGridSize - 1; ++x)
		{
			_sharedChunkData.textureCreationTris->push_back((glbase::TriangleSet::value_type)((y + 0) * splatGridSize + (x + 0)));
			_sharedChunkData.textureCreationTris->push_back((glbase::TriangleSet::value_type)((y + 1) * splatGridSize + (x + 0)));
			_sharedChunkData.textureCreationTris->push_back((glbase::TriangleSet::value_type)((y + 1) * splatGridSize + (x + 1)));
			_sharedChunkData.textureCreationTris->push_back((glbase::TriangleSet::value_type)((y + 0) * splatGridSize + (x + 0)));
			_sharedChunkData.textureCreationTris->push_back((glbase::TriangleSet::value_type)((y + 1) * splatGridSize + (x + 1)));
			_sharedChunkData.textureCreationTris->push_back((glbase::TriangleSet::value_type)((y + 0) * splatGridSize + (x + 1)));
		}
	}
	_sharedChunkData.textureCreationTris->sync_all();

	set_state(State::Empty);
	CHECK_OPENGL_ERRORS;

	init_adjacency();

	create_distant_geom(distantGeometry, parent);
}

void PlanetGeometry::create_distant_geom(const render::GeometrySet::ptr& distantGeometry, const scene::transform::Group::ptr& parent)
{
	struct PlanetBillboardVert
	{
		Vector3f vert;
		Vector2f uv;
		Vector2f offset;
	};

	scene::Geometry::ptr mesh(new scene::Geometry());

	scene::Material::ptr mat(new scene::Material());
	mat->set_effect(_planetBillboardShader);
	mat->set_parameter("Texture", _planetBillboardTexture);
	mat->set_parameter("MinimumSize", 2.0f);
	mat->set_parameter("Size", _sharedChunkData.radius);
	mesh->set_material(mat);

	glbase::VertexSet::ptr verts(new glbase::VertexSet(_planetBillboardVertexSpec, 4));
	static const float uvs[][2] = {{0, 0}, {0, 1}, {1, 1}, {1, 0}};
	//float size = 8;
	for(size_t idx = 0; idx < 4; ++idx)
	{
		PlanetBillboardVert* vert = verts->extract<PlanetBillboardVert>(idx);
		vert->vert = Vector3f::Zero;
		vert->uv = Vector2f(uvs[idx]);
		vert->offset = (Vector2f(uvs[idx]) - 0.5f) * 2.0f;
	}
	verts->sync_all();
	mesh->set_verts(verts);

	glbase::TriangleSet::ptr tris(new glbase::TriangleSet(glbase::TrianglePrimitiveType::TRIANGLES));
	tris->push_back(0);
	tris->push_back(1);
	tris->push_back(2);
	tris->push_back(0);
	tris->push_back(2);
	tris->push_back(3);
	tris->sync_all();
	mesh->set_tris(tris);
	mesh->set_transform(parent);
	distantGeometry->insert(mesh);
}

PlanetGeometry::PlanetGeometryBuildData::PlanetGeometryBuildData(HeightDataProvider::ptr heightData)
: _heightData(heightData)
{

}

void PlanetGeometry::PlanetGeometryBuildData::init(size_t gridSize, float radius, bool water, float waterRadius, 
										   const Transform::matrix4_type& rootTransform)
{
	reset_flags();
	_gridSize = gridSize;
	_radius = radius;
	_rootTransform = rootTransform;
	_waterRadius = waterRadius;
	_water = water;
	_pts.resize(0);

	_workRemaining = _totalWork = _gridSize * _gridSize * 6;
}

void PlanetGeometry::PlanetGeometryBuildData::process(size_t step)
{
	createSphere(_gridSize, _radius, _water, _waterRadius, _rootTransform, *_heightData, _pts);
	_workRemaining = 0;
}

size_t PlanetGeometry::PlanetGeometryBuildData::get_total_work() const
{
	return _totalWork;
}

size_t PlanetGeometry::PlanetGeometryBuildData::get_work_remaining() const
{
	return _workRemaining;
}

PlanetGeometry::PlanetTextureBuildData::PlanetTextureBuildData(HeightDataProvider::ptr heightData,
													   PlanetTextureBlender::ptr textureData)
	: _heightData(heightData),
	_textureData(textureData)
{

}

void PlanetGeometry::PlanetTextureBuildData::init(size_t textureSize, float radius)
{
	reset_flags();
	_textureSize = textureSize;
	_radius = radius;
	_totalWork = _workRemaining = _textureSize * _textureSize * 6;
}

void PlanetGeometry::PlanetTextureBuildData::process(size_t step)
{
	using namespace math;

	//std::vector<unsigned char> bits(_textureSize * _textureSize * 4);
	std::vector<Vector3f> heightbits((_textureSize + 2) * (_textureSize + 2));

	std::vector< Vector3f > orientations;
	orientations.push_back(Vector3f(0.0f,	0.0f,	0.0f));   // front
	orientations.push_back(Vector3f(0.0f,	180.0f, 0.0f)); // back
	orientations.push_back(Vector3f(0.0f,	-90.0f, 0.0f)); // left
	orientations.push_back(Vector3f(0.0f,	90.0f,	0.0f));  // right
	orientations.push_back(Vector3f(-90.0f, 0.0f,	0.0f)); // top
	orientations.push_back(Vector3f(90.0f,	0.0f,	0.0f));  // bottom

	_normals.resize(6);
	_heights.resize(6);
	_colours.resize(6);

	for(size_t idx = 0; idx < 6; ++idx)
	{
		_normals[idx].resize(_textureSize * _textureSize * 3);
		_colours[idx].resize(_textureSize * _textureSize * 3);
		_heights[idx].resize(_textureSize * _textureSize);
		createTextureSide(_colours[idx], *_heightData, *_textureData, 
			heightbits, _normals[idx],	_heights[idx], orientations[idx], _radius, _textureSize);
	}
	_workRemaining = 0;
}

size_t PlanetGeometry::PlanetTextureBuildData::get_total_work() const
{
	return _totalWork;
}

size_t PlanetGeometry::PlanetTextureBuildData::get_work_remaining() const
{
	return _workRemaining;
}


void PlanetGeometry::PlanetChunksBuildData::init(size_t gridSize, const PlanetGeometryBuildData::ptr& geometry)
{
	_gridSize = gridSize;
	_geometry = geometry;
	_workRemaining = _totalWork = gridSize * gridSize * 6;
}

void PlanetGeometry::PlanetChunksBuildData::process(size_t step)
{
	size_t chunksGridSize = _gridSize-1;

	_chunkPartitionItems.resize(6 * chunksGridSize * chunksGridSize);
	ChunkSpatialPartition::ItemPtrSet chunkPartitionItems;
	for(size_t faceIdx = 0, chunkPartIdx = 0; faceIdx < 6; ++faceIdx)
	{
		for(size_t y = 0; y < chunksGridSize; ++y)
		{
			for(size_t x = 0; x < chunksGridSize; ++x, ++chunkPartIdx)
			{
				ChunkPartitionItem& item = _chunkPartitionItems[chunkPartIdx];
				item.faceIdx = faceIdx;
				item.faceCoords = math::Vector2i(x, y);

				std::vector<size_t>& vertIndices = item.verts;
				AABBf& aabb = item.bounds;
				for(size_t qy = 0; qy < 2; ++qy)
				{
					for(size_t qx = 0; qx < 2; ++qx)
					{
						size_t idx = _geometry->get_pt_idx(
							x + qx, y + qy, 0, faceIdx);
						vertIndices.push_back(idx);
						const PlanetSurfaceVertex& pt = _geometry->get_pt(idx);
						aabb.expand(pt.pos);
					}
				}

				chunkPartitionItems.insert(&_chunkPartitionItems[chunkPartIdx]);
			}
		}
	}

	_chunkPartitions.create(chunkPartitionItems);	
	_workRemaining = 0;
}

size_t PlanetGeometry::PlanetChunksBuildData::get_total_work() const
{
	return _totalWork;
}

size_t PlanetGeometry::PlanetChunksBuildData::get_work_remaining() const
{
	return _workRemaining;
}

void PlanetGeometry::update(scene::transform::Camera::ptr camera, const scene::transform::Transform::vec3_type& cameraVelocitySeconds)
{
	using namespace math;
	// determine if we should show the 3D model based on the camera settings,
	// distance of camera from planet, radius of planet..
	Vector3d camVec(camera->centerGlobal() - _sharedChunkData.rootTransform->centerGlobal());
	float dist = (float)camVec.length();

	Vector3d projCenter(camera->globalise(Transform::vec3_type(0.0f, 0.0f, -dist)));
	Vector3d projRight(camera->globalise(Transform::vec3_type(_sharedChunkData.radius, 0.0f, -dist)));
	// project planet radius using camera, to determine area of screen covered by planet
	Vector3d unprojCenter(camera->project_global(projCenter)); 
	Vector3d unprojRight(camera->project_global(projRight));

	float pixelRadius = (float)(Vector2d(unprojRight) - Vector2d(unprojCenter)).length();
	float pixelArea = static_cast<float>(M_PI * (pixelRadius * pixelRadius));

	if(pixelArea > _pixelAreaToActivateChunks)
	{
		if(set_state(State::ChunksActive))
			update_chunks(camera, cameraVelocitySeconds);
	}
	else if(pixelArea > _pixelAreaToShow)
	{
		set_state(State::Shown);
	}
	else if(pixelArea > _pixelAreaToCreate)
	{
		set_state(State::Prepared);
	}
	else
	{
		set_state(State::Empty);
	}

	if(_state.get_state() >= State::Prepared)
	{
		check_texture_update();
		//check_atmosphere_update();
	}
}

PlanetGeometry::DeferredCollision::ptr PlanetGeometry::collide(Transform::vec3_type pt, double minRange) const
{
	HeightDataProvider::pts_vector_type vec;
	vec.push_back(HeightDataProvider::vector_type(pt.normal()));
	HeightDataProvider::NoiseGen::NoiseExecutionPtr exec = _sharedChunkData.heightData->queue(vec);
	return DeferredCollision::ptr(new DeferredCollision(_sharedChunkData.radius, minRange, exec, pt));
	//pt = _sharedChunkData.rootTransform->localise(pt);
	//
	//HeightDataProvider::pts_vector_type vec;
	//vec.push_back(HeightDataProvider::vector_type(pt));
	//((double)_sharedChunkData.heightData->
	//HeightDataProvider::value_type fullheight = (HeightDataProvider::value_type)
	//	((double)_sharedChunkData.heightData->get(HeightDataProvider::Vector3type(pt.normal()), true) + _sharedChunkData.radius + (double)minRange);
	//if(pt.lengthSquared() < Transform::float_type(fullheight * fullheight))
	//	pt = pt.normal() * Transform::float_type(fullheight);
	//return _sharedChunkData.rootTransform->globalise(pt);
}

scene::transform::Group::ptr PlanetGeometry::get_parent_transform() const
{
	return _sharedChunkData.rootTransform;
}

size_t PlanetGeometry::get_curr_max_chunk_depth() const
{
	size_t maxDepth = 0;

	for(ChunkPartitionMap::const_iterator cItr = _activeChunks.begin();
		cItr != _activeChunks.end(); ++cItr)
	{
		maxDepth = std::max(maxDepth, cItr->second.chunk->get_curr_max_depth());
	}

	return maxDepth;
}


void PlanetGeometry::init_state()
{
	_state.add_transition_up_func	(State::Empty,			std::bind(&PlanetGeometry::prepare, this));
	_state.add_transition_up_func	(State::Prepared,		std::bind(&PlanetGeometry::show, this));
	_state.add_transition_up_func	(State::Shown,			std::bind(&PlanetGeometry::create_chunks, this));
	_state.add_transition_down_func	(State::ChunksActive,	std::bind(&PlanetGeometry::delete_chunks, this));
	_state.add_transition_down_func	(State::Shown,			std::bind(&PlanetGeometry::hide, this));
	_state.add_transition_down_func	(State::Prepared,		std::bind(&PlanetGeometry::destroy, this));
}

bool PlanetGeometry::set_state(State::type newState)
{
	return _state.set_state(newState);
}

bool PlanetGeometry::prepare()	// Empty -> Prepared
{
	if(_building)
	{
		check_built();
		return _built; // double check as "check_built" updates built
	}

	_building = true;

	assert(!_geometryThreadData);
	assert(!_textureThreadData);

	_geometryThreadData.reset(new PlanetGeometryBuildData(_sharedChunkData.heightData));
	_textureThreadData.reset(new PlanetTextureBuildData(_sharedChunkData.heightData, _sharedChunkData.textureData));

	_geometryThreadData->init(_gridSize, _sharedChunkData.radius, _water, _sharedChunkData.waterRadius, _sharedChunkData.rootTransform->globalTransform());
	DataDistributeThread::get_instance()->queue_data(_geometryThreadData);

	_currentTextureSize = 0;
	_textureThreadData->init(_initalTextureSize, _sharedChunkData.radius);
	DataDistributeThread::get_instance()->queue_data(_textureThreadData);
	_buildingTexture = true;

	if(_atmosphere)
		_atmosphere->prepare_for_draw();

	//math::Atmospheref atmos;
	//atmos.set_radii(_sharedChunkData.radius, _atmosphere->atmosphereRadius);
	//atmos.set_scale_height(_atmosphere->scaleHeight);
	//atmos.set_k(_atmosphere->n, _atmosphere->Ns);
	//std::vector<math::Vector3f> Is, wavelengths;
	//for(size_t idx = 0; idx < _atmosphere->suns.size(); ++idx)
	//{
	//	scene::transform::Light::ptr light = _atmosphere->suns[idx]->get_light();
	//	Is.push_back(light->get_color());
	//	//wavelengths.push_back(light->get_wavelength());
	//}
	//_atmosphereThreadData.init(atmos, _atmosphere->opticalDepthTextureSize, 
	//	_atmosphere->surfaceLightingTextureSize, _atmosphere->opticalDepthSamples, 
	//	_atmosphere->surfaceLightingSamples, _atmosphere->KFrThetaOverLambda4Samples, 
	//	Is, _atmosphere->mieU);
	//DataDistributeThread::get_instance()->queue_data(&_atmosphereThreadData);
	//_buildingAtmosphere = true;

	assert(_built == false); // this should already be true

	return false;
}

bool PlanetGeometry::show()		// Prepared -> Shown
{
	for(size_t idx = 0; idx < 6; ++idx)
	{
		FaceData& face = _faces[idx];
		_sharedChunkData.geometry->insert(face.mesh);
	}

	return true;
}

bool PlanetGeometry::create_chunks() // Shown -> ChunksActive
{
	if(_creatingChunks)
	{
		check_chunks_built();
		return _chunksBuilt;
	}

	_creatingChunks = true;

	assert(_built);
	_chunkThreadData.reset(new PlanetChunksBuildData());
	_chunkThreadData->init(_gridSize, _geometryThreadData);
	DataDistributeThread::get_instance()->queue_data(_chunkThreadData);

	return false;
}

bool PlanetGeometry::delete_chunks() // ChunksActive -> Shown
{
	assert(_chunksBuilt);

	_activeChunks.clear();

	abort_chunks_build();

	return true;
}

bool PlanetGeometry::hide()		// Shown -> Prepared
{
 	assert(_built);
	for(size_t idx = 0; idx < 6; ++idx)
	{
		FaceData& face = _faces[idx];
		_sharedChunkData.geometry->erase(face.mesh);
	}
	return true;
}

bool PlanetGeometry::destroy()	// Prepared -> Empty
{
	// need to abort all chunk creation, as they try to reference planet vertex data
	abort_chunks_build();
	//_chunkThreadData = PlanetChunksBuildData();

	abort_build();
	//_geometryThreadData = PlanetGeometryBuildData(_sharedChunkData.heightData);
	//_textureThreadData = PlanetTextureBuildData(_sharedChunkData.heightData, _sharedChunkData.textureData);

	for(size_t idx = 0; idx < 6; ++idx)
	{
		_faces[idx] = FaceData();
	}

	return true;
}

void PlanetGeometry::abort_build()
{
	// abort geometry
	if(_geometryThreadData)
		_geometryThreadData->set_abort_build();
	_geometryThreadData.reset();
	_building = _built = false;

	// abort texture
	if(_textureThreadData)
		_textureThreadData->set_abort_build();
	_textureThreadData.reset();
	_buildingTexture = false;
	_currentTextureSize = 0;
}

void PlanetGeometry::check_built()
{
	if(_built)
		return;
	if(!_building)
		return;
	if(_geometryThreadData->is_finished_build() && _textureThreadData->is_finished_build() && (!_atmosphere || _atmosphere->is_ready_to_draw()))
	{
		load_geometry_from_thread_data();
		load_textures_from_thread_data();
		//load_atmosphere_from_thread_data();
		_built = true;
		_building = _buildingTexture = false;
	}
}

void PlanetGeometry::abort_chunks_build()
{
	if(_chunkThreadData)
		_chunkThreadData->set_abort_build();
	_chunkThreadData.reset(); // free here, if it is still building the build thread will hold it
	_creatingChunks = _chunksBuilt = false;
}

void PlanetGeometry::check_chunks_built()
{
	if(_chunksBuilt) 
		return;
	if(!_creatingChunks)
		return;
	if(_chunkThreadData->is_finished_build())
	{
		_chunksBuilt = true;
		_creatingChunks = false;
	}
}


void PlanetGeometry::check_texture_update()
{
	if(_currentTextureSize < _targetTextureSize)
	{
		if(_buildingTexture)
		{
			if(_textureThreadData->is_finished_build())
			{
				load_textures_from_thread_data();
				_buildingTexture = false;
			}
		}
		else
		{
			size_t nextTextureSize = _currentTextureSize * 2;
			assert(_currentTextureSize <= _targetTextureSize);
			_textureThreadData->init(nextTextureSize, _sharedChunkData.radius);
			DataDistributeThread::get_instance()->queue_data(_textureThreadData);
			_buildingTexture = true;
		}
	}
}

// void PlanetGeometry::check_atmosphere_update()
// {
// 	if(_atmosphere->is_building() && _atmosphere->is_ready_to_draw())
// 	{
// 		//load_atmosphere_from_thread_data();
// 		_buildingAtmosphere = false;
// 	}
// }

void PlanetGeometry::load_geometry_from_thread_data()
{
	using namespace scene;

	glbase::VertexSet::ptr verts;
	// copy the vertex set
	verts.reset(new glbase::VertexSet(_vertexSpec, _geometryThreadData->get_pts_count(), GL_STREAM_DRAW));
	memcpy_s(&(*verts)(0), verts->get_local_buffer_size_bytes(), _geometryThreadData->get_data(), _geometryThreadData->get_pts_count() * _vertexSpec->vertexSize());
	verts->sync_all();
	// create the tri meshes, copy the tri strip data across and attach the verts/material/etc
	assert(verts->get_count() < std::numeric_limits<glbase::TriangleSet::value_type>::max());

	std::vector< tri_index_type > tristrip;
	for(size_t idx = 0; idx < 6; ++idx)
	{
		FaceData& face = _faces[idx];
		face.mesh.reset(new Geometry());
		face.mesh->set_transform(_sharedChunkData.rootTransform);
		face.mesh->set_verts(verts);
		Material::ptr material(new Material());
		material->set_effect(_surfaceShader);
		face.mesh->set_material(material);

		face.ptIdxOffset = _gridSize * _gridSize * idx;
		tristrip.resize(0);
		buildTristrip(_gridSize, face.ptIdxOffset, std::unordered_set< math::Vector2i >(), tristrip);
		glbase::TriangleSet::ptr tris(new glbase::TriangleSet(glbase::TrianglePrimitiveType::TRIANGLE_STRIP, tristrip.size()));
		memcpy_s(&(*tris)(0), tris->get_local_buffer_size_bytes(), &tristrip[0], tristrip.size() * sizeof(tri_index_type));
		tris->sync_all();
		face.mesh->set_tris(tris);
	}
}

void PlanetGeometry::load_textures_from_thread_data()
{
	_currentTextureSize = _textureThreadData->get_texture_size();

	size_t chunksGridSize = _gridSize-1;

	for(size_t faceIdx = 0; faceIdx < 6; ++faceIdx)
	{
		FaceData& face = _faces[faceIdx];
		update_UVs(_gridSize, face.ptIdxOffset, _geometryThreadData->get_data(), _currentTextureSize);
		face.normalTexture.reset(new glbase::Texture());
		face.normalTexture->create2D(GL_TEXTURE_2D, (GLuint)_currentTextureSize, (GLuint)_currentTextureSize, 3, 
			GL_RGB, GL_UNSIGNED_BYTE, _textureThreadData->get_normal_texture(faceIdx), GL_RGB8);
		face.mesh->get_material()->set_parameter("NormalTexture", face.normalTexture);
		face.heightTexture.reset(new glbase::Texture());
		face.heightTexture->create2D(GL_TEXTURE_2D, (GLuint)_currentTextureSize, (GLuint)_currentTextureSize, 1, 
			GL_RED, GL_FLOAT, _textureThreadData->get_height_texture(faceIdx), GL_R32F);
		face.mesh->get_material()->set_parameter("HeightTexture", face.heightTexture);
		face.colourTexture.reset(new glbase::Texture());
		face.colourTexture->create2D(GL_TEXTURE_2D, (GLuint)_currentTextureSize, (GLuint)_currentTextureSize, 3, 
			GL_RGB, GL_UNSIGNED_BYTE, _textureThreadData->get_colour_texture(faceIdx), GL_RGB8);
		face.mesh->get_material()->set_parameter("ColourTexture", face.colourTexture);
	}

	for(ChunkPartitionMap::iterator cItr = _activeChunks.begin(); 
		cItr != _activeChunks.end(); ++cItr)
	{
		ChunkPartitionItem* pItem = cItr->first;
		ChunkLOD::ptr chunk = cItr->second.chunk;

		const PlanetSurfaceVertex& topLeft = _geometryThreadData->get_pt(
			pItem->faceCoords.x, 
			pItem->faceCoords.y, 0, pItem->faceIdx);
		const PlanetSurfaceVertex& bottomRight = _geometryThreadData->get_pt(
			pItem->faceCoords.x + 1, 
			pItem->faceCoords.y + 1, 0, pItem->faceIdx);

		math::Rectanglef chunkUVs(topLeft.uv.x, bottomRight.uv.y, 
			bottomRight.uv.x, topLeft.uv.y);

		FaceData& face = _faces[pItem->faceIdx];
		chunk->update_parent_textures(face.normalTexture, face.colourTexture, chunkUVs);
	}

	glbase::VertexSet::ptr verts = _faces[0].mesh->get_verts();
	memcpy_s(&(*verts)(0), verts->get_local_buffer_size_bytes(), _geometryThreadData->get_data(), _geometryThreadData->get_pts_count() * _vertexSpec->vertexSize());
	verts->sync_all();
}

void PlanetGeometry::createSphere(size_t gridSize, float radius, bool water, float waterRadius,
						  const Transform::matrix4_type& rootTransform,
						  HeightDataProvider& heightData,
						  std::vector<PlanetSurfaceVertex>& pts)
{
	using namespace math;
	using namespace scene;
	using namespace std;

	// create sphere by:
	// create 8 point cube
	// rd = 90deg / gridSize;
	// create edge points between the cube points including them in the list:
	// norm = pt0.cross(pt1) < save this with the edge pts
	// for i = 0 to gridSize, r = 0, r += rd
	//   m = rotateaa(norm, r)
	//   edgept = pt0 x m
	// endfor
	// create cube faces from sets of edges
	// cubeface(ledge, redge, tedge, bedge) << use iterators so edges can be reversed
	// 
	// center is assumed to be origin
	// for each side:
	//   tl = top left pt, bl = bottom left pt
	//   tr = top right pt, br = bottom right pt
	//   y goes from top to bottom
	//   x goes from left to right
	//   for y = 0 to gridSize, x norm = tl.cross(bl)

	Vector3f corners[8];

	float size = radius * (1/Vector3f::One.length());
	corners[0] = Vector3f(-size,  size,  size); // l t n
	corners[1] = Vector3f( size,  size,  size); // r t n
	corners[2] = Vector3f(-size, -size,  size); // l b n
	corners[3] = Vector3f( size, -size,  size); // r b n
	corners[4] = Vector3f( size,  size, -size); // r t f
	corners[5] = Vector3f(-size,  size, -size); // l t f
	corners[6] = Vector3f( size, -size, -size); // r b f
	corners[7] = Vector3f(-size, -size, -size); // l b f

	vector< Vector3f > edges[8];
	// create gridSized sets of vertices separated equally by angle
	createEdge((int)gridSize, corners[0], corners[2], edges[0]); // left front
	createEdge((int)gridSize, corners[1], corners[3], edges[1]); // right front
	createEdge((int)gridSize, corners[4], corners[6], edges[2]); // right back
	createEdge((int)gridSize, corners[5], corners[7], edges[3]); // left back
	createEdge((int)gridSize, corners[5], corners[0], edges[4]); // left top
	createEdge((int)gridSize, corners[7], corners[2], edges[5]); // left bottom
	createEdge((int)gridSize, corners[1], corners[4], edges[6]); // right top
	createEdge((int)gridSize, corners[3], corners[6], edges[7]); // right bottom

	HeightDataProvider::pts_vector_type normalPts;
	createFace(edges[0], false, edges[1], false, normalPts, pts); // front
	createFace(edges[2], false, edges[3], false, normalPts, pts); // back
	createFace(edges[3], false, edges[0], false, normalPts, pts); // left
	createFace(edges[1], false, edges[2], false, normalPts, pts); // right
	createFace(edges[4], false, edges[6], true,  normalPts, pts); // top
	createFace(edges[5], true,  edges[7], false, normalPts, pts); // bottom

	HeightDataProvider::results_vector_type heights = heightData.get(normalPts, false);
	for(size_t idx = 0; idx < heights.size(); ++idx)
	{
		//float poslen = pts[idx*2].pos.length();
		float height = /*water? std::max<float>(radius + heights[idx], waterRadius) :*/ radius + std::max<float>(0, heights[idx] * (float)heightData.scale());
		// set skirt vert first
		pts[idx*2+1].pos = math::Vector3f(normalPts[idx] * (height - (float)heightData.scale()));
		pts[idx*2].pos = math::Vector3f(normalPts[idx] * height);
	}
}

void PlanetGeometry::update_UVs(size_t gridSize, size_t ptIdxOffset,
					   PlanetSurfaceVertex* pts, 
					   size_t textureSize)
{
	float uvOffset = 0.5f/static_cast<float>(textureSize);
	float v = uvOffset, duv = (1 / static_cast<float>(gridSize-1))
		* ((textureSize-1) / static_cast<float>(textureSize));
	for(size_t y = 0, idx = ptIdxOffset * 2; y < gridSize; ++y, v += duv)
	{
		float u = uvOffset;
		for(size_t x = 0; x < gridSize; ++x, u += duv, idx+=2)
		{
			PlanetSurfaceVertex& pVert = pts[idx];
			pVert.uv = Vector2f(u, v);
			PlanetSurfaceVertex& pVert2 = pts[idx+1];
			pVert2.uv = Vector2f(u, v);
		}
	}
}

void PlanetGeometry::createFace(const std::vector< Vector3f >& left, bool revL, 
						const std::vector< Vector3f >& right, bool revR,
						HeightDataProvider::pts_vector_type& normalPts, 
						std::vector< PlanetSurfaceVertex >& pts)
{
	using namespace math;
	using namespace scene;
	using namespace std;

	// assert the dimensions are correct
	assert(left.size() == right.size());

	size_t gridSize = left.size();

	Vector3f tlpt = left[0];
	Vector3f blpt = left[gridSize-1];
	float yfullangle = rad_to_deg(tlpt.normal().angle(blpt.normal()));

	size_t ptIdxOffset = pts.size() / 2;
	for(size_t y = 0; y < gridSize; ++y)
	{
		Vector3f startpt = revL ? left[gridSize - 1 - y] : left[y];
		Vector3f endpt = revR ? right[gridSize - 1 - y] : right[y];
		Vector3f raxis = startpt.crossp(endpt).normal();
		float xfullangle = rad_to_deg(startpt.normal().angle(endpt.normal()));

		float dxangle = xfullangle / (gridSize-1);
		float xangle = 0;
		for(size_t x = 0; x < gridSize; ++x, xangle += dxangle)
		{
			Matrix4f rmat = math::rotate_axis_angle(raxis, xangle);
			PlanetSurfaceVertex pVert(Vector3f(rmat * (Vector4f(startpt) + Vector4f::WAxis)), 
				Vector2f());
			pts.push_back(pVert); // twice as the second one is the skirt vertex, to be filled in later
			pts.push_back(pVert); // but we don't want to resize and copy array contents again
			normalPts.push_back(HeightDataProvider::pts_vector_type::value_type(pVert.pos.normal()));
		}
	}
}

tri_index_type PlanetGeometry::buildTristrip(size_t gridSize, size_t ptIdxOffset, 
						   const std::unordered_set< math::Vector2i  >& gaps,
						   std::vector< tri_index_type >& tristrip)
{
	bool skipping = true, started = false;

	tri_index_type lastidx = 0;
	for(tri_index_type y = 0, idx = 0; y < gridSize-1; ++y)
	{
		for(tri_index_type x = 0; x < gridSize-1; ++x, ++idx)
		{
			bool skip = gaps.find(math::Vector2i(x,y)) != gaps.end();
			if(!skip)
			{
				if(skipping)
				{
					if(started)
					{
						tristrip.push_back(lastidx);
						tristrip.push_back((unsigned short)(idx + 0 + 0 * gridSize + ptIdxOffset) * 2);
					}
					tristrip.push_back((unsigned short)(idx + 0 + 0 * gridSize + ptIdxOffset) * 2);
					tristrip.push_back((unsigned short)(idx + 0 + 1 * gridSize + ptIdxOffset) * 2);
				}
				tristrip.push_back((unsigned short)(idx + 1 + 0 * gridSize + ptIdxOffset) * 2);
				lastidx = (tri_index_type)(idx + 1 + 1 * gridSize + ptIdxOffset) * 2;
				tristrip.push_back(lastidx);

				started = true;
			}
			skipping = true; 
		}
		// skip past the last vertex as we already created the triangles that use it
		++ idx;
		skipping = true;
	}
	return lastidx;
}

void PlanetGeometry::buildSkirts(size_t gridSize, size_t ptIdxOffset, tri_index_type lastpt, const std::unordered_set< math::Vector2i >& gaps, std::vector< tri_index_type >& triStrip)
{
	if(!gaps.empty())
	{
		triStrip.push_back(lastpt);
		const math::Vector2i& firstChunk = *(gaps.begin());
		lastpt = (tri_index_type)(firstChunk.y * gridSize + firstChunk.x + ptIdxOffset) * 2;
		triStrip.push_back(lastpt);
	}

	for(auto gItr = gaps.begin(); gItr != gaps.end(); ++gItr)
	{
		const math::Vector2i& chunk = *gItr;
		// start degenerate triangle from last point
		triStrip.push_back(lastpt);
		lastpt = (tri_index_type)(((chunk.y + 0) * gridSize + chunk.x + 0 + ptIdxOffset) * 2) + 0;
		// close degenerate triangle from last point
		triStrip.push_back(lastpt);
		// start new triangle
		triStrip.push_back(lastpt);

		// top left
		triStrip.push_back((unsigned short)(((chunk.y + 0) * gridSize + chunk.x + 0 + ptIdxOffset) * 2) + 1);
		// bottom left
		triStrip.push_back((unsigned short)(((chunk.y + 1) * gridSize + chunk.x + 0 + ptIdxOffset) * 2) + 0);
		triStrip.push_back((unsigned short)(((chunk.y + 1) * gridSize + chunk.x + 0 + ptIdxOffset) * 2) + 1);
		// bottom right
		triStrip.push_back((unsigned short)(((chunk.y + 1) * gridSize + chunk.x + 1 + ptIdxOffset) * 2) + 0);
		triStrip.push_back((unsigned short)(((chunk.y + 1) * gridSize + chunk.x + 1 + ptIdxOffset) * 2) + 1);
		// top right
		triStrip.push_back((unsigned short)(((chunk.y + 0) * gridSize + chunk.x + 1 + ptIdxOffset) * 2) + 0);
		triStrip.push_back((unsigned short)(((chunk.y + 0) * gridSize + chunk.x + 1 + ptIdxOffset) * 2) + 1);
		// top left again
		triStrip.push_back((unsigned short)lastpt); // reuse as it we already calculated the top left pt
		lastpt = (unsigned short)(((chunk.y + 0) * gridSize + chunk.x + 0 + ptIdxOffset) * 2) + 1;
		triStrip.push_back((unsigned short)lastpt);
	}
}

void PlanetGeometry::createEdge(int ptCount, const Vector3f& startpt, const Vector3f& endpt, 
				std::vector< Vector3f >& verts, int border)
{
	assert(border >= 0);

	using namespace math;
	Vector3f raxis = startpt.crossp(endpt).normal();
	float fullangle = math::rad_to_deg(startpt.normal().angle(endpt.normal()));

	float ad = fullangle / (ptCount-1);
	// create intermediate pts
	for(int idx = -border; idx < ptCount+border; ++idx)
	{
		float angle = ad * idx;
		Matrix4f rmat = math::rotate_axis_angle(raxis, angle);
		verts.push_back(Vector3f(rmat * (Vector4f(startpt) + Vector4f::WAxis)));
	}
	// set end pt index
}

size_t get_height_idx(int x, int y, int textureSize)
{
	return (y + 1) * (textureSize + 2) + x + 1;
}

size_t get_normal_colour_idx(int x, int y, int textureSize)
{
	return (y * textureSize + x) * 3;
}

void PlanetGeometry::createTextureSide(std::vector<unsigned char>& colourTextureData, 
							   HeightDataProvider& heightDataProvider,
							   const PlanetTextureBlender& textureDataProvider,
							   std::vector<math::Vector3f>& positionData, 
							   std::vector<unsigned char>& normalTextureData, 
							   std::vector<float>& heightData, 
							   const math::Vector3f& orientation,
							   float radius,
							   size_t textureSize)
{
	using namespace math;
	Matrix4f rotm = rotate_euler(orientation);
	float texelSize = 1.0f / textureSize;
	HeightDataProvider::pts_vector_type vs((textureSize + 2) * (textureSize + 2));

	Vector3f tl(matrix_transform(rotm, Vector3f(-1.0f, 1.0f, 1.0f))), 
		tr(matrix_transform(rotm, Vector3f(1.0f, 1.0f, 1.0f))), 
		bl(matrix_transform(rotm, Vector3f(-1.0f, -1.0f, 1.0f))), 
		br(matrix_transform(rotm, Vector3f(1.0f, -1.0f, 1.0f)));

	std::vector< Vector3f > leftSide, rightSide, horiz;
	createEdge((int)textureSize, tl, bl, leftSide, 1);
	createEdge((int)textureSize, tr, br, rightSide, 1);
	for(int y = 0, idx = 0; y < textureSize + 2; ++y)
	{
		Vector3f& leftPt = leftSide[y];
		Vector3f& rightPt = rightSide[y];
		horiz.resize(0);
		createEdge((int)textureSize, leftPt, rightPt, horiz, 1);

		for(int x = 0; x < textureSize + 2; ++x, ++idx)
		{
			vs[idx] = HeightDataProvider::vector_type(horiz[x].normal());
		}
	}

	HeightDataProvider::results_vector_type heights = heightDataProvider.get(vs, false);
	
	for(unsigned int hbitidx = 0; hbitidx < heights.size(); ++hbitidx)
	{
		float height = radius + std::max<float>(0, heights[hbitidx] * (float)heightDataProvider.scale());
		positionData[hbitidx] = math::Vector3f(vs[hbitidx]) * height;
	}

	math::SSETraits<float>::Vector polarAngles, internalHeights, verticalAngles;
	for(int y = 0; y < textureSize; ++y)
	{
		for(int x = 0; x < textureSize; ++x)
		{
			Vector3f& origin = positionData[get_height_idx(x, y, (int)textureSize)];
			Vector3f down(positionData[get_height_idx(x, y+1, (int)textureSize)] - origin);
			Vector3f up(positionData[get_height_idx(x, y-1, (int)textureSize)] - origin);
			Vector3f right(positionData[get_height_idx(x+1, y, (int)textureSize)] - origin);
			Vector3f left(positionData[get_height_idx(x-1, y, (int)textureSize)] - origin);

			Vector3f norm1 = down.crossp(right);
			Vector3f norm2 = right.crossp(up);
			Vector3f norm3 = up.crossp(left);
			Vector3f norm4 = left.crossp(down);
			Vector3f norm = (norm1 + norm2 + norm3 + norm4).normal();
			size_t ncIdx = get_normal_colour_idx(x, y, (int)textureSize);
			normalTextureData[ncIdx+0] = static_cast<unsigned char>(norm.x * 128 + 127);
			normalTextureData[ncIdx+1] = static_cast<unsigned char>(norm.y * 128 + 127);
			normalTextureData[ncIdx+2] = static_cast<unsigned char>(norm.z * 128 + 127);

			size_t heightIdx = get_height_idx(x, y, (int)textureSize);
			internalHeights.push_back(heights[heightIdx]);
			heightData[y * textureSize + x] = radius + std::max<float>(0, heights[heightIdx] * (float)heightDataProvider.scale());

			polarAngles.push_back(origin.angle(Vector3f::YAxis));
			verticalAngles.push_back(norm.angle(origin.normal()));
		}
	}

	assert(internalHeights.size() == polarAngles.size());
	assert(internalHeights.size() == verticalAngles.size());

	textureDataProvider.get_texture_colours(internalHeights, polarAngles, 
		verticalAngles, colourTextureData);
}

void PlanetGeometry::activate_chunk(ChunkPartitionItem* chunkData)
{
	using namespace scene;

	math::Vector3f center = chunkData->bounds.center();
	transform::Group::ptr chunkGroup(new transform::Group());
	//_sharedChunkData.rootTransform->insert(chunkGroup);
	//chunkGroup->setParent(_sharedChunkData.rootTransform.get()); don't do this, chunkGroup is planet local
	Transform::matrix4_type chunkGroupOffset(translate(Transform::vec3_type(center)));
	Transform::matrix4_type chunkGroupOffsetInverse(chunkGroupOffset.inverse());
	chunkGroup->setTransform(chunkGroupOffset);
	std::vector<math::Vector3f> sourcePts(chunkData->verts.size());
	for(size_t idx = 0; idx < chunkData->verts.size(); ++idx)
	{
		const PlanetSurfaceVertex& pt = _geometryThreadData->get_pt(chunkData->verts[idx]);
		sourcePts[idx] = math::Vector3f(chunkGroupOffsetInverse * Transform::vec4_type(pt.pos, 1.0f));
	}
	_sharedChunkData.rootTransform->insert(chunkGroup);

	const PlanetSurfaceVertex& topLeft = _geometryThreadData->get_pt(
		chunkData->faceCoords.x, 
		chunkData->faceCoords.y, 0, chunkData->faceIdx);
	const PlanetSurfaceVertex& bottomRight = _geometryThreadData->get_pt(
		chunkData->faceCoords.x + 1, 
		chunkData->faceCoords.y + 1, 0, chunkData->faceIdx);

	math::Rectanglef chunkUVs(topLeft.uv.x, bottomRight.uv.y, 
		bottomRight.uv.x, topLeft.uv.y);

	FaceData& face = _faces[chunkData->faceIdx];
	ChunkLOD::ptr newChunk(new ChunkLOD(&_sharedChunkData, chunkGroup, sourcePts, face.normalTexture, 
		face.colourTexture, chunkUVs));

	_activeChunks.insert(std::make_pair(chunkData, 
		ChunkData(newChunk, chunkData->faceCoords)));
}

void PlanetGeometry::deactivate_chunk(ChunkPartitionItem* chunkData)
{
	ChunkPartitionMap::iterator mItr = _activeChunks.find(chunkData);
	assert(mItr != _activeChunks.end());
	ChunkLOD::delete_chunk(mItr->second.chunk);
	_activeChunks.erase(mItr);
}

bool PlanetGeometry::update_chunk(ChunkData& chunkData, const scene::transform::Camera::ptr& camera, 
								  scene::transform::Transform::vec3_type predictedCameraPos, std::unordered_set< Vector2i >& shownChunks)
{
	ChunkLOD::ptr chunk = chunkData.chunk;
	math::Vector3f cameraPosLocal(chunk->get_root_transform()->localise(camera->centerGlobal()));
	math::Vector3f cameraPredictedPosLocal(chunk->get_root_transform()->localise(camera->globalise(predictedCameraPos)));
	bool wasShown = chunk->is_shown_or_children_shown();
	chunk->update(cameraPosLocal, cameraPredictedPosLocal);
	bool nowShown = chunk->is_shown_or_children_shown();
	if(nowShown)
	{
		shownChunks.insert(chunkData.loc);
	}

	return wasShown != nowShown;
}

void PlanetGeometry::update_chunks(const scene::transform::Camera::ptr& camera, const scene::transform::Transform::vec3_type& cameraVelocitySeconds)
{
	ChunkSpatialPartition::ItemPtrSet newActiveChunks; 

	// determine the set of chunks that should be active
	math::Vector3f cameraPosLocal(_sharedChunkData.rootTransform->localise(camera->centerGlobal()));
	_chunkThreadData->get_chunk_partitions().get_items_in_range(cameraPosLocal, 
		_chunkActivationDistance, newActiveChunks);

	// activate any chunks that were not previously active
	for(ChunkSpatialPartition::ItemPtrSet::iterator cItr = newActiveChunks.begin(); 
		cItr != newActiveChunks.end(); ++cItr)
	{
		ChunkPartitionItem* item = *cItr;
		ChunkPartitionMap::iterator fItr = _activeChunks.find(item);
		if(fItr == _activeChunks.end())
		{
			activate_chunk(item);
		}
	}

	// guess the position of the camera by the time it would take to create geometry for the chunk
	// limit the time in case the timings get messed up somehow, to force creation of at least some block
	float timeToCreateChunk = std::min<float>(utils::Profiler::get_timing_ms(ChunkLOD::CREATE_GEOMETRY_PROFILE, 5) * 0.001f, 0.5f);
	// multiply time by a factor as we want the block to be worth creating
	scene::transform::Transform::vec3_type predictedCameraPos = /*camera->center() +*/ cameraVelocitySeconds * (double)timeToCreateChunk * 1.0;

	std::unordered_set< Vector2i > shownChunks[6];
	std::unordered_set< ChunkPartitionItem* > deactivateSet, changedSet;
	bool shownChanged[6] = {false, false, false, false, false, false};
	// deactivate any chunks that should no longer be active
	for(auto cItr = _activeChunks.begin(); 
		cItr != _activeChunks.end(); ++cItr)
	{
		ChunkPartitionItem* item = cItr->first;
		ChunkSpatialPartition::ItemPtrSet::iterator fItr = newActiveChunks.find(item);
		if(fItr == newActiveChunks.end())
		{
			deactivateSet.insert(cItr->first);
		}
		else
		{
			bool chunkChanged = update_chunk(cItr->second, camera, predictedCameraPos, shownChunks[item->faceIdx]);
			shownChanged[item->faceIdx] = shownChanged[item->faceIdx] || chunkChanged;
			if(chunkChanged)
				changedSet.insert(cItr->first);
		}
	}

	for(auto dItr = deactivateSet.begin(); 
		dItr != deactivateSet.end(); ++dItr)
	{
		ChunkPartitionItem* item = *dItr;
		// make sure faces that had active 
		if(item->faceCoords.x == 0)
			shownChanged[_adjaceny[item->faceIdx].left.faceIndex] = true;
		else if(item->faceCoords.x == _gridSize - 2)
			shownChanged[_adjaceny[item->faceIdx].right.faceIndex] = true;
		if(item->faceCoords.y == 0)
			shownChanged[_adjaceny[item->faceIdx].top.faceIndex] = true;
		else if(item->faceCoords.y == _gridSize - 2)
			shownChanged[_adjaceny[item->faceIdx].bottom.faceIndex] = true;
		shownChanged[item->faceIdx] = true;
		deactivate_chunk(*dItr);
	}

	// deactivated chunks require adjacency calculation as well, so that skirts can
	// be removed from their affected faces.
	// effected faces change flags must be set for all that are changing
	// just update all faces if anything changes? 

	// determine border chunks
	std::unordered_set< Vector2i > borderChunks[6];
	// add all border chunks
	for(auto cItr = _activeChunks.begin(); 
		cItr != _activeChunks.end(); ++cItr)
	{
		ChunkPartitionItem* item = cItr->first;
		ChunkData& chunkData = cItr->second;

		if(!chunkData.chunk->is_shown_or_children_shown())
			continue;

		math::Vector2i faceCoord = item->faceCoords;
		// left border
		Vector2i left(faceCoord.x - 1, faceCoord.y);
		size_t leftFaceIndex = item->faceIdx, rightFaceIndex = item->faceIdx, 
			topFaceIndex = item->faceIdx, bottomFaceIndex = item->faceIdx;
		if(left.x == -1)
		{
			left.x = int(_gridSize) - 2;
			left = Vector2i(_adjaceny[item->faceIdx].left.rot * Vector3i(left, 1));
			leftFaceIndex = _adjaceny[item->faceIdx].left.faceIndex;
		}
		// right border
		Vector2i right(faceCoord.x + 1, faceCoord.y);
		if(right.x == int(_gridSize) - 1)
		{
			right.x = 0;
			right = Vector2i(_adjaceny[item->faceIdx].right.rot * Vector3i(right, 1));
			rightFaceIndex = _adjaceny[item->faceIdx].right.faceIndex;
		}
		// top border
		Vector2i top(faceCoord.x, faceCoord.y - 1);
		if(top.y == -1)
		{
			top.y = int(_gridSize) - 2;
			top = Vector2i(_adjaceny[item->faceIdx].top.rot * Vector3i(top, 1));
			topFaceIndex = _adjaceny[item->faceIdx].top.faceIndex;
		}
		// bottom border
		Vector2i bottom(faceCoord.x, faceCoord.y + 1);
		if(bottom.y == int(_gridSize) - 1)
		{
			bottom.y = 0;
			bottom = Vector2i(_adjaceny[item->faceIdx].bottom.rot * Vector3i(bottom, 1));
			bottomFaceIndex = _adjaceny[item->faceIdx].bottom.faceIndex;
		}
		borderChunks[leftFaceIndex].insert(left);
		borderChunks[rightFaceIndex].insert(right);
		borderChunks[topFaceIndex].insert(top);
		borderChunks[bottomFaceIndex].insert(bottom);
		if(changedSet.find(item) != changedSet.end())
		{
			shownChanged[leftFaceIndex] = true;
			shownChanged[rightFaceIndex] = true;
			shownChanged[topFaceIndex] = true;
			shownChanged[bottomFaceIndex] = true;
		}
	}
	// remove all active chunks from border chunks
	for(auto cItr = _activeChunks.begin(); 
		cItr != _activeChunks.end(); ++cItr)
	{
		ChunkPartitionItem* item = cItr->first;
		ChunkData& chunkData = cItr->second;

		if(chunkData.chunk->is_shown_or_children_shown())
			borderChunks[item->faceIdx].erase(item->faceCoords);
	}

	for(size_t idx = 0; idx < 6; ++idx)
	{
		if(shownChanged[idx])
		{
			FaceData& face = _faces[idx];
			std::vector< tri_index_type > triStrip;
			tri_index_type lastPt = buildTristrip(_gridSize, face.ptIdxOffset, shownChunks[idx], triStrip);
			buildSkirts(_gridSize, face.ptIdxOffset, lastPt, borderChunks[idx], triStrip);
			glbase::VertexSet::ptr verts = face.mesh->get_verts();
			glbase::TriangleSet::ptr tris(new glbase::TriangleSet(glbase::TrianglePrimitiveType::TRIANGLE_STRIP, triStrip.size()));
			if(triStrip.size() != 0)
				memcpy_s(&((*tris)(0)), tris->get_local_buffer_size_bytes(), &(triStrip[0]), sizeof(tri_index_type) * triStrip.size());
			tris->sync_all();
			face.mesh->set_tris(tris);
		}
	} 

	ChunkLOD::delete_aborted_chunks();

	//FramebufferObject::Disable();
}

void PlanetGeometry::static_init()
{
	_surfaceShader.reset(new effect::Effect());
	_surfaceShader->load("../Data/Shaders/planet_surface.xml");
	_textureCreationShader.reset(new effect::Effect());
	_textureCreationShader->load("../Data/Shaders/lod_chunk_splat_shader.xml");

	_planetBillboardShader.reset(new effect::Effect());
	_planetBillboardShader->load("../Data/Shaders/planet_billboard.xml");

	_planetBillboardVertexSpec.reset(new glbase::VertexSpec());
	_planetBillboardVertexSpec->append(glbase::VertexData::PositionData, 0, sizeof(float), 3, glbase::VertexElementType::Float);
	_planetBillboardVertexSpec->append(glbase::VertexData::TexCoord0, 1, sizeof(float), 2, glbase::VertexElementType::Float);
	_planetBillboardVertexSpec->append(glbase::VertexData::TexCoord1, 2, sizeof(float), 2, glbase::VertexElementType::Float);

	_planetBillboardTexture.reset(new glbase::Texture());
	_planetBillboardTexture->load("../Data/PlanetTextures/billboard.png");

	ChunkLOD::static_init();
}

void PlanetGeometry::static_release()
{
	ChunkLOD::static_release();

	_surfaceShader.reset();
	_textureCreationShader.reset();
	_planetBillboardShader.reset();
	_planetBillboardVertexSpec.reset();
	_planetBillboardTexture.reset();
}

float PlanetGeometry::get_max_height() const
{
	return _sharedChunkData.radius * (1.0f + _heightScaleFactor);
}

effect::Effect::ptr PlanetGeometry::_surfaceShader;
effect::Effect::ptr PlanetGeometry::_textureCreationShader;
glbase::Texture::ptr PlanetGeometry::_planetBillboardTexture;
glbase::VertexSpec::ptr PlanetGeometry::_planetBillboardVertexSpec;
effect::Effect::ptr PlanetGeometry::_planetBillboardShader;

Transform::vec3_type PlanetGeometry::DeferredCollision::get() const
{
	HeightDataProvider::NoiseGen::NoiseExecution::results_vector_type results = exec->get_results();
	double fullHeight = results.front() + radius + minRange;

	Transform::vec3_type ptret = localpt;
	if(ptret.lengthSquared() < fullHeight * fullHeight)
		ptret = ptret.normal() * fullHeight;

	return ptret;
}

PlanetGeometry::DeferredCollision::DeferredCollision( double radius_, double minRange_, const HeightDataProvider::NoiseGen::NoiseExecutionPtr& exec_, const Transform::vec3_type& localpt_ ) 
	: radius(radius_),
	minRange(minRange_),
	exec(exec_),
	localpt(localpt_)
{
}

}