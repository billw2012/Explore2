// solution
#include "fbo\framebufferObject.h"
#include "Math\misc.hpp"
#include "Math\matrix4.hpp"
#include "Math\transformation.hpp"
#include "Scene\material.hpp"
#include "Render\utils_screen_space.h"
#include "GLbase\videomemorymanager.hpp"

// project
#include "element.h"
#include "renderer.h"

namespace vice {;

/*

Rendering component instance:
Requires two full size textures to render to, with depth buffers.
Background of UI should already be in these textures for composition purposes.

Depth buffers act as clipping for each element, with depths assigned to elements matching
Rendering order (from front to back).



*/

using namespace glbase;

namespace {;

Texture::ptr get_stencil_texture(const Texture::ptr& texture)
{
	struct VecHash
	{
		size_t operator()(const math::Vector2i& vec) const 
		{
			std::hash<int> intHash;
			return intHash(vec.x) ^ intHash(vec.y);
		}
	};
	static std::unordered_map<math::Vector2i, Texture::ptr, VecHash> sStencilTextures;
	math::Vector2i dims(texture->width(), texture->height());
	auto itr = sStencilTextures.find(dims);
	if(itr == sStencilTextures.end())
	{
		Texture::ptr depth = std::make_shared<Texture>();
		depth->create2D(GL_TEXTURE_2D, texture->width(), texture->height(), 1, GL_DEPTH_COMPONENT, GL_UNSIGNED_INT, nullptr, GL_DEPTH24_STENCIL8);
		CHECK_OPENGL_ERRORS;
		sStencilTextures[dims] = depth;
		return depth;
	}
	return itr->second;
}

struct Buffer
{
	Texture::ptr tex, depth;
	static FramebufferObject::ptr fbo;

	void init(const Texture::ptr& texsrc)
	{
		if(!fbo)
		{
			fbo = std::make_shared<FramebufferObject>();
		}
		tex = texsrc;
		fbo->AttachTexture(tex->handle(), GL_COLOR_ATTACHMENT0);
		CHECK_OPENGL_ERRORS;
		depth = get_stencil_texture(tex);
		fbo->AttachTexture(depth->handle(), GL_DEPTH_STENCIL_ATTACHMENT);
		CHECK_OPENGL_ERRORS;

		assert(fbo->IsValid());
	}
};

FramebufferObject::ptr Buffer::fbo;

struct CopyBuffer
{
	Buffer writeBuffer;
	Texture::ptr background;

	void init(const Texture::ptr& target, const Texture::ptr& background)
	{
		writeBuffer.init(target);
		this->background = background;
	}

	void copy(int x, int y, int width, int height)
	{
		using namespace math;
		x = clamp<int>(x, 0, background->width());
		y = clamp<int>(y, 0, background->height());
		width = clamp<int>(width, 0, background->width() - x);
		height = clamp<int>(height, 0, background->height() - y);

		glCopyImageSubData(writeBuffer.tex->handle(), GL_TEXTURE_RECTANGLE, 0, x, y, 0, background->handle(), GL_TEXTURE_RECTANGLE, 0, x, y, 0, width, height, 1);
		CHECK_OPENGL_ERRORS;
	}

	void back_copy(int x, int y, int width, int height)
	{
		using namespace math;
		x = clamp<int>(x, 0, background->width());
		y = clamp<int>(y, 0, background->height());
		width = clamp<int>(width, 0, background->width() - x);
		height = clamp<int>(height, 0, background->height() - y);

		glCopyImageSubData(background->handle(), GL_TEXTURE_RECTANGLE, 0, x, y, 0, writeBuffer.tex->handle(), GL_TEXTURE_RECTANGLE, 0, x, y, 0, width, height, 1);
		CHECK_OPENGL_ERRORS;
	}

	void copy()
	{
		copy(0, 0, background->width(), background->height());
	}

	void back_copy()
	{
		back_copy(0, 0, background->width(), background->height());
	}
};

// TODO: cache copy buffers between frames
CopyBuffer get_copy_buffer(const Texture::ptr& target, const Texture::ptr& background)
{
	CopyBuffer buff;
	buff.init(target, background);
	return buff;
}

struct RenderNode
{
	typedef std::shared_ptr<RenderNode> ptr;
	const Object* object;

	RenderNode(const Object* object_ = nullptr) : object(object_) {}

	struct RenderNodePtrSort
	{
		bool operator()(const RenderNode::ptr& lhs, const RenderNode::ptr& rhs) const
		{
			if(lhs->object->get_order() < rhs->object->get_order())
				return true;
			if(lhs->object->get_order() > rhs->object->get_order())
				return false;
			return lhs < rhs;
		}
	};
	std::set<RenderNode::ptr, RenderNodePtrSort> children;
};

void build_component_heirarchy(const ComponentInstance* comp, const RenderNode::ptr& compNode)
{
	std::unordered_map<const Object*, RenderNode::ptr> nodeMap;

	//compNode->object = comp;
	//nodeMap[comp] = compNode;

	for(int idx = 0; idx < comp->get_child_count(); ++idx)
	{
		auto child = comp->get_child(idx);

		RenderNode::ptr node = std::make_shared<RenderNode>(child.get());

		if(child->get_type() == Object::ComponentType)
		{
			build_component_heirarchy(static_cast<ComponentInstance*>(child.get()), node);
		}
		else
		{
			//child->get_parent();
		}
		nodeMap[child.get()] = node;
	}

	for(auto itr = nodeMap.begin(); itr != nodeMap.end(); ++itr)
	{
		std::string parent = itr->first->get_parent();
		if(parent.empty())
			compNode->children.insert(itr->second);
		else
		{
			auto parentObj = comp->get_child_by_name(parent);
			nodeMap[parentObj.get()]->children.insert(itr->second);
		}
	}
}

scene::Material::ptr get_material(const Object* /*obj*/)
{
	// TODO: shader set by element etc.
	static scene::Material::ptr sMaterial;
	static effect::Effect::ptr sEffect;
	if(!sMaterial)
	{
		sMaterial = std::make_shared<scene::Material>();
		sEffect = std::make_shared<effect::Effect>();
		if(!sEffect->load("../data/vice/shaders/element_shader.xml"))
			std::cout << "Error: " << sEffect->get_last_error() << std::endl;
		sMaterial->set_effect(sEffect);
	}
	return sMaterial;
}

scene::Material::ptr get_selection_material()
{
	// TODO: shader set by element etc.
	static scene::Material::ptr sMaterial;
	static effect::Effect::ptr sEffect;
	if (!sMaterial)
	{
		sMaterial = std::make_shared<scene::Material>();
		sEffect = std::make_shared<effect::Effect>();
		if (!sEffect->load("../data/vice/shaders/selection_shader.xml"))
			std::cout << "Error: " << sEffect->get_last_error() << std::endl;
		sMaterial->set_effect(sEffect);
	}
	return sMaterial;
}

scene::Material::ptr get_stencil_material()
{
	// TODO: shader set by element etc.
	static scene::Material::ptr sMaterial;
	static effect::Effect::ptr sEffect;
	if (!sMaterial)
	{
		sMaterial = std::make_shared<scene::Material>();
		sEffect = std::make_shared<effect::Effect>();
		if (!sEffect->load("../data/vice/shaders/stencil_shader.xml"))
			std::cout << "Error: " << sEffect->get_last_error() << std::endl;
		sMaterial->set_effect(sEffect);
	}
	return sMaterial;
}

void draw_rect(float x, float y, float width, float height)
{
	static scene::Geometry::ptr sRect;
	if(!sRect)
	{
		sRect = render::utils::create_new_screen_quad(x, y, width, height);
		CHECK_OPENGL_ERRORS;
	}
	else
	{
		render::utils::update_screen_quad(sRect, x, y, width, height);
		CHECK_OPENGL_ERRORS;
	}
	if(glbase::VideoMemoryManager::load_buffers(sRect->get_tris(), sRect->get_verts()))
	{
		CHECK_OPENGL_ERRORS;
		glbase::VideoMemoryManager::render_current();
		CHECK_OPENGL_ERRORS;
		glbase::VideoMemoryManager::unbind_buffers();
		CHECK_OPENGL_ERRORS;
	}
}

void draw_text(float x, float y, float width, float height, const std::string& text)
{

}

/*
 
 Bind element shader
 If element shader uses background image:
 Bind background to shader
 If background is dirty (is it always dirty?)
 Copy renderbuffer to background, maybe clipped to this node bounds
 Bind depth to shader
 currtransform = transform * node.object.transform
 Bind currtransform to shader

 Set stencil op: draw if greater or equal to depth, increment succeed
 Draw element background, text, border, etc.

 Increase depth

 for each child in node.children
	DrawNode(child, depth, currtransform)
 
 Decrease depth

 Set stencil op: draw if greater than depth, set to depth on succeed
 Disable color write
 Redraw element background
 */

template < typename Ty_ >
Ty_ interpolate(Ty_ a, Ty_ b, Ty_ s)
{
	return a + (b - a) * s;
}

Color rotate_color(const Color& colStart, const Color& colEnd)
{
	using namespace std::chrono;
	typedef duration<float> fsec;

	static high_resolution_clock clock;
	static auto start = clock.now();
	fsec t = clock.now() - start;
	float intpart;
	float frac = std::modf(t.count() * 2.0f, &intpart);
	float interpVal = std::fabs((frac * 2) - 1);
	return Color(
		interpolate(colStart.r, colEnd.r, interpVal),
		interpolate(colStart.g, colEnd.g, interpVal),
		interpolate(colStart.b, colEnd.b, interpVal),
		interpolate(colStart.a, colEnd.a, interpVal));
}

void draw_node(const ComponentInstance::LayoutNode& node, int depth, const math::Matrix4f& camera, CopyBuffer& buffer)
{
	static const GLuint MASKON = ~(GLuint)0U;
	static const GLuint MASKOFF = (GLuint)0U;
	static const Color SELECTED_COLOR_START(1, 1, 0, 0.5f), SELECTED_COLOR_END(1, 1, 1, 0.5f);
	static const Color HIGHLIGHTED_COLOR_START(0, 1, 0, 0.5f), HIGHLIGHTED_COLOR_END(0.7f, 1, 0.7f, 0.5f);

	// TODO: copy only required rectangle
	buffer.copy();

	auto object = node.object;

	// increment the stencil value if the test passes
	glStencilOp(GL_KEEP, GL_INCR, GL_INCR);
	// only draw where stencil value is greater or equal to depth, so we cull everything outside the parent
	glStencilFunc(GL_LEQUAL, depth, MASKON);
	
	math::Matrix4f transform = camera * object->get_global_transform();

	if(object->get_type() == Object::ElementType)
	{
		auto element = static_cast<const Element*>(object);
		glStencilMask(MASKON);

		// bind shader
		scene::Material::ptr material = get_material(object);
		material->set_parameter("background", buffer.background);
		Color color = element->get_color();
		material->set_parameter("color", math::Vector4f(color.r, color.g, color.b, color.a));
		material->set_parameter("MODELVIEWPROJMAT", transform);
		material->bind();
		CHECK_OPENGL_ERRORS;
		// draw element
		draw_rect(0, 0, element->get_width(), element->get_height());
		// only want to update stencil for the background
		
		glStencilMask(MASKOFF);
		draw_text(0, 0, element->get_width(), element->get_height(), element->get_text());
		
		scene::Material::unbind();
		CHECK_OPENGL_ERRORS;
		// update the background
		buffer.copy();
	}
	else // a component, still need to update the stencil though
	{
		glStencilMask(MASKON);
		// bind a material just for drawing stencil values
		scene::Material::ptr material = get_stencil_material();
		material->set_parameter("MODELVIEWPROJMAT", transform);
		material->bind();	
		CHECK_OPENGL_ERRORS;
		draw_rect(0, 0, object->get_width(), object->get_height());
		scene::Material::unbind();
		CHECK_OPENGL_ERRORS;
	}

	// draw children
	for (auto itr = node.children.begin(); itr != node.children.end(); ++itr)
	{
		draw_node(**itr, depth + 1, camera, buffer);
	}

	if (object->is_selected() || object->is_highlighted())
	{
		glDisable(GL_STENCIL_TEST);

		// bind shader
		scene::Material::ptr material = get_selection_material();
		material->set_parameter("background", buffer.background);
		Color color = object->is_selected()? 
			rotate_color(SELECTED_COLOR_START, SELECTED_COLOR_END) :
			rotate_color(HIGHLIGHTED_COLOR_START, HIGHLIGHTED_COLOR_END);
		material->set_parameter("color", math::Vector4f(color.r, color.g, color.b, color.a));
		material->set_parameter("MODELVIEWPROJMAT", transform);
		material->bind();
		CHECK_OPENGL_ERRORS;

		//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

		// draw selection border
		draw_rect(0, 0, object->get_width(), object->get_height());

		//glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);

		scene::Material::unbind();
		CHECK_OPENGL_ERRORS;

		glDisable(GL_STENCIL_TEST);
	}

	// clear our stencil changes
	glStencilOp(GL_KEEP, GL_REPLACE, GL_REPLACE);
	glStencilFunc(GL_LESS, depth, MASKON);
	glStencilMask(MASKON);
	// bind a material just for drawing stencil values
	scene::Material::ptr material = get_stencil_material();
	material->set_parameter("MODELVIEWPROJMAT", transform);
	material->bind();	
	draw_rect(0, 0, object->get_width(), object->get_height());
	scene::Material::unbind();
}

void render_component_root( const ComponentInstance& component, CopyBuffer& buffer )
{
	int depth = 0;
	math::Matrix4f camera = component.get_camera();
	draw_node(component.get_layout_root(), depth, camera, buffer);
}

};

void vice::Renderer::render( const ComponentInstance& component )
{
	auto target = component.get_texture();
	auto background = component.get_background();

	assert(background->width() == target->width());
	assert(background->height() == target->height());
	assert(background->format() == target->format());
	assert(background->type() == target->type());
	assert(background->components() == target->components());

	CopyBuffer copyBuffer = get_copy_buffer(target, background);

	copyBuffer.back_copy();

	glEnable(GL_STENCIL_TEST);
	glViewport(0, 0, target->width(), target->height());
	CHECK_OPENGL_ERRORS;
	copyBuffer.writeBuffer.fbo->Bind();
	CHECK_OPENGL_ERRORS;
	copyBuffer.writeBuffer.fbo->BindDrawBuffers();
	CHECK_OPENGL_ERRORS;
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT | GL_STENCIL_BUFFER_BIT);
	CHECK_OPENGL_ERRORS;
	render_component_root(component, copyBuffer);
	FramebufferObject::Disable();
	CHECK_OPENGL_ERRORS;
	glDisable(GL_STENCIL_TEST);
}

}
