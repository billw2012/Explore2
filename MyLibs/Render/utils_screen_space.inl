
namespace render {;
namespace utils {;

template < class Ty_ >
inline scene::Geometry::ptr create_new_screen_quad( Ty_ x, Ty_ y, Ty_ width, Ty_ height, UVType::type uvType /*= UVType::Pixel*/ )
{
	using namespace scene;
	using namespace math;
	using namespace glbase;

	TriangleSet::ptr triSet(new TriangleSet(TrianglePrimitiveType::TRIANGLE_STRIP, 0, GL_STREAM_DRAW));
	triSet->push_back(0);
	triSet->push_back(1);
	triSet->push_back(2);
	triSet->push_back(3);
	triSet->sync_all();

	// create the vertex spec for the mesh
	VertexSpec::ptr vertSpec(new VertexSpec());
	vertSpec->append(VertexData::PositionData, 0, sizeof(float), 2, VertexElementType::Float);
	vertSpec->append(VertexData::TexCoord0, 1, sizeof(float), 2, VertexElementType::Float);

	// create the verts for the mesh
	VertexSet::ptr verts(new VertexSet(vertSpec, 4, GL_STREAM_DRAW));

	assert(sizeof(QuadVert) == vertSpec->vertexSize());
	if(uvType == UVType::Relative)
	{
		// top left vert
		*verts->extract<QuadVert>(0) = QuadVert(math::Vector2f(x, y), math::Vector2f(0, 0));
		// bottom left vert
		*verts->extract<QuadVert>(1) = QuadVert(math::Vector2f(x, y+height), math::Vector2f(0, 1));
		// top right
		*verts->extract<QuadVert>(2) = QuadVert(math::Vector2f(x+width, y), math::Vector2f(1, 0));
		// bottom right
		*verts->extract<QuadVert>(3) = QuadVert(math::Vector2f(x+width, y+height), math::Vector2f(1, 1));
	}
	else
	{
		// top left vert
		*verts->extract<QuadVert>(0) = QuadVert(math::Vector2f(x, y), math::Vector2f(x, y));
		// bottom left vert
		*verts->extract<QuadVert>(1) = QuadVert(math::Vector2f(x, y+height), math::Vector2f(x, y+height));
		// top right
		*verts->extract<QuadVert>(2) = QuadVert(math::Vector2f(x+width, y), math::Vector2f(x+width, y));
		// bottom right
		*verts->extract<QuadVert>(3) = QuadVert(math::Vector2f(x+width, y+height), math::Vector2f(x+width, y+height));
	}

	verts->sync_all();

	return Geometry::ptr(new Geometry(triSet, verts, scene::Material::ptr(), transform::Transform::ptr(new transform::Transform())));
}

template < class Ty_ >
inline void update_screen_quad( const scene::Geometry::ptr& geom, Ty_ x, Ty_ y, Ty_ width, Ty_ height, UVType::type uvType /*= UVType::Pixel*/ )
{
	using namespace scene;
	using namespace math;
	using namespace glbase;

	const glbase::VertexSet::ptr& verts = geom->get_verts();

	assert(verts->get_count() == 4);

	if(uvType == UVType::Relative)
	{
		// top left vert
		*verts->extract<QuadVert>(0) = QuadVert(math::Vector2f(x, y), math::Vector2f(0, 0));
		// bottom left vert
		*verts->extract<QuadVert>(1) = QuadVert(math::Vector2f(x, y+height), math::Vector2f(0, 1));
		// top right
		*verts->extract<QuadVert>(2) = QuadVert(math::Vector2f(x+width, y), math::Vector2f(1, 0));
		// bottom right
		*verts->extract<QuadVert>(3) = QuadVert(math::Vector2f(x+width, y+height), math::Vector2f(1, 1));
	}
	else
	{
		// top left vert
		*verts->extract<QuadVert>(0) = QuadVert(math::Vector2f(x, y), math::Vector2f(x, y));
		// bottom left vert
		*verts->extract<QuadVert>(1) = QuadVert(math::Vector2f(x, y+height), math::Vector2f(x, y+height));
		// top right
		*verts->extract<QuadVert>(2) = QuadVert(math::Vector2f(x+width, y), math::Vector2f(x+width, y));
		// bottom right
		*verts->extract<QuadVert>(3) = QuadVert(math::Vector2f(x+width, y+height), math::Vector2f(x+width, y+height));
	}

	verts->sync_all();
}

}
}