#if !defined(__EFFECT_GLSTATE_H__)
#define __EFFECT_GLSTATE_H__

#include <unordered_map>
#include <unordered_set>
#include <vector>
#include <sstream>

//#define BOOST_BIND_ENABLE_STDCALL



#include <boost/algorithm/string.hpp>

#include <GL/glew.h>

/*
GL_ALPHA_TEST
If enabled, do alpha testing. 
glAlphaFunc
	GL_NEVER 
	Never passes. 
	GL_LESS 
	Passes if the incoming alpha value is less than the reference 
	value. 
	GL_EQUAL 
	Passes if the incoming alpha value is equal to the reference 
	value. 
	GL_LEQUAL 
	Passes if the incoming alpha value is less than or equal to the 
	reference value. 
	GL_GREATER 
	Passes if the incoming alpha value is greater than the reference 
	value. 
	GL_NOTEQUAL 
	Passes if the incoming alpha value is not equal to the reference 
	value. 
	GL_GEQUAL 
	Passes if the incoming alpha value is greater than or equal to the
	reference value. 
	GL_ALWAYS 
	Always passes (initial value). 

GL_BLEND
If enabled, blend the incoming RGBA color values with the values in the 
color buffers. 
glBlendFunc
	GL_ZERO (0, 0, 0, 0) 
	GL_ONE (1, 1, 1, 1) 
	GL_SRC_COLOR (Rs/kR, Gs/kG, Bs/kB, As/kA) 
	GL_ONE_MINUS_SRC_COLOR (1, 1, 1, 1)  (Rs/kR, Gs/kG, Bs/kB, As/kA ) 
	GL_DST_COLOR (Rd/kR, Gd/kG, Bd/kB, Ad/kA) 
	GL_ONE_MINUS_DST_COLOR (1, 1, 1, 1)  (Rd/kR, Gd/kG, Bd/kB, Ad/kA ) 
	GL_SRC_ALPHA (As/kA, As/kA, As/kA, As/kA) 
	GL_ONE_MINUS_SRC_ALPHA (1, 1, 1, 1)  (As/kA, As/kA, As/kA, As/kA ) 
	GL_DST_ALPHA (Ad/kA, Ad/kA, Ad/kA, Ad/kA) 
	GL_ONE_MINUS_DST_ALPHA (1, 1, 1, 1)  (Ad/kA, Ad/kA, Ad/kA, Ad/kA ) 
	GL_SRC_ALPHA_SATURATE (i, i, i, 1) 

GL_CULL_FACE
If enabled, cull polygons based on their winding in window coordinates. 
glCullFace
	GL_FRONT, 
	GL_BACK, and 
	GL_FRONT_AND_BACK

GL_DEPTH_TEST
If enabled, do depth comparisons and update the depth buffer. Note that 
even if the depth buffer exists and the depth mask is non-zero, the 
depth buffer is not updated if the depth test is disabled.  
glDepthFunc
	GL_NEVER 
	Never passes. 
	GL_LESS 
	Passes if the incoming depth value is less than the stored depth 
	value. 
	GL_EQUAL 
	Passes if the incoming depth value is equal to the stored depth 
	value. 
	GL_LEQUAL 
	Passes if the incoming depth value is less than or equal to the 
	stored depth value. 
	GL_GREATER 
	Passes if the incoming depth value is greater than the stored 
	depth value. 
	GL_NOTEQUAL 
	Passes if the incoming depth value is not equal to the stored 
	depth value. 
	GL_GEQUAL 
	Passes if the incoming depth value is greater than or equal to the
	stored depth value. 
	GL_ALWAYS 
	Always passes. 
glDepthRange
	zNear 
	Specifies the mapping of the near clipping plane to window 
	coordinates. The initial value is 0. 
	zFar 
	Specifies the mapping of the far clipping plane to window 
	coordinates. The initial value is 1. 

//GL_LINE_SMOOTH
//If enabled, draw lines with correct filtering. Otherwise, draw aliased 
//lines. See glLineWidth.
//
//GL_LINE_STIPPLE
//If enabled, use the current line stipple pattern when drawing lines. 
//See glLineStipple.
//
//GL_NORMALIZE
//If enabled, normal vectors specified with glNormal are scaled to unit 
//length after transformation. See glNormal.
//
//GL_POINT_SMOOTH
//If enabled, draw points with proper filtering. Otherwise, draw aliased 
//points. See glPointSize.
//
//GL_POLYGON_OFFSET_FILL
//If enabled, and if the polygon is rendered in GL_FILL mode, an offset 
//is added to depth values of a polygon's fragments before the depth
//comparison is performed. See glPolygonOffset.
//
//GL_POLYGON_OFFSET_LINE
//If enabled, and if the polygon is rendered in GL_LINE mode, an offset 
//is added to depth values of a polygon's fragments before the depth
//comparison is performed. See glPolygonOffset.
//
//GL_POLYGON_OFFSET_POINT
//If enabled, an offset is added to depth values of a polygon's fragments
//before the depth comparison is performed, if the polygon is rendered 
//in GL_POINT mode. See glPolygonOffset.
//
//GL_POLYGON_SMOOTH
//If enabled, draw polygons with proper filtering. Otherwise, draw 
//aliased polygons. For correct anti-aliased polygons, an alpha buffer 
//is needed and the polygons must be sorted front to back.
//
//GL_POLYGON_STIPPLE
//If enabled, use the current polygon stipple pattern when rendering 
//polygons. See glPolygonStipple.
//
//GL_SCISSOR_TEST
//If enabled, discard fragments that are outside the scissor rectangle.
//See glScissor.
//
//GL_STENCIL_TEST
//If enabled, do stencil testing and update the stencil buffer. See 
//glStencilFunc and glStencilOp.
*/

namespace effect {;

/*

Read name string, set of value strings.
Choose value parsers based on name from a map.
Give value parser vector of strings.
Value parser parses strings and binds them to its function call.

*/

struct GLState
{
	typedef std::shared_ptr < GLState > ptr;

	typedef std::function< void () > StateFn;

	struct StateSetResetPair
	{
		StateSetResetPair(StateFn set_ = StateFn(), StateFn reset_ = StateFn(), size_t stateID_ = 0, const std::string& valuesID_ = "error") 
			: set(set_), reset(reset_), stateID(stateID_), valuesID(valuesID_) {}
		StateFn set;
		StateFn reset;
		size_t stateID;
		std::string valuesID;

		bool operator<(const StateSetResetPair& other) const
		{
			//return stateID < other.stateID;
			if(stateID < other.stateID) return true;
			if(stateID > other.stateID) return false;
			return valuesID < other.valuesID;
			//if(set < other.set) return true;
			//if(set > other.set) return false;
			//return reset < other.reset;
		}
		bool operator==(const StateSetResetPair& other) const
		{
			return stateID == other.stateID && valuesID == other.valuesID;
		}
	};

	struct StateSetResetPairHasher
	{
		size_t operator()(const StateSetResetPair& val) const 
		{
			return val.stateID ^ std::hash<std::string>()(val.valuesID);;
		}
	};

	struct StateSetResetPairUniqueHasher
	{
		size_t operator()(const StateSetResetPair& val) const 
		{
			return val.stateID;
		}
	};

	struct StateSetResetPairUniqueCompare
	{
		size_t operator()(const StateSetResetPair& lhs, const StateSetResetPair& rhs) const 
		{
			return lhs.stateID == rhs.stateID;
		}
	};

	typedef std::unordered_set< StateSetResetPair, StateSetResetPairHasher > GLStateUniqueSet;
	typedef std::unordered_set< StateSetResetPair, StateSetResetPairUniqueHasher, StateSetResetPairUniqueCompare > GLStateSet;

	GLState(StateFn defaultState) 
		: _defaultStateFn(defaultState), _stateID(++_stateIDCounter) {}

	virtual StateSetResetPair parse(const std::vector<std::string>& params) const = 0;

	StateFn get_default_state_fn() const { return _defaultStateFn; }

protected: // parsing functions

	size_t get_state_ID() const { return _stateID; }

	static bool parse_bool(std::string str)
	{
		str = boost::to_lower_copy(boost::trim_copy(str));

		if(str == "true" || str == "gl_true")
			return true;
		if(str == "false" || str == "gl_false")
			return false;

		throw std::exception( (std::string("Error: could not parse state string, "
			"should be true/gl_true/false/gl_false, was ") + str).c_str() );
	}

	static float parse_float(const std::string& str)
	{
		std::stringstream ss(str);

		float fv;
		ss >> fv;

		return fv;
	}

private:
	StateFn _defaultStateFn;
	size_t _stateID;
	static size_t _stateIDCounter;
};

struct StateTypes
{
	typedef std::unordered_map< std::string, GLState::ptr > NameStateMap;

private:
	// states:
	struct GLState_Boolean : GLState
	{
		GLState_Boolean(GLenum enumVal, bool enabled = false) 
			: GLState(enabled ? std::bind(::glEnable, enumVal) : 
			std::bind(::glDisable, enumVal)), _enumVal(enumVal) {}

		virtual StateSetResetPair parse(const std::vector<std::string>& params) const
		{
			if(params.size() != 1)
				throw std::exception("Error: glEnable/glDisable takes one parameter.");

			if(parse_bool(params[0]))
				return StateSetResetPair(std::bind(::glEnable, _enumVal), get_default_state_fn(), get_state_ID(), "true");
			else
				return StateSetResetPair(std::bind(::glDisable, _enumVal), get_default_state_fn(), get_state_ID(), "false");
		}

	private:
		GLenum _enumVal;
	};

	struct EnumNamePair
	{
		EnumNamePair(GLenum enumVal_ = 0) : enumVal(enumVal_)
		{
			std::stringstream ss;
			ss << enumVal_;
			enumName = ss.str();
		}
		EnumNamePair(GLenum enumVal_, const std::string& enumName_) 
			: enumVal(enumVal_), enumName(enumName) {}

		GLenum enumVal;
		std::string enumName;
	};
	struct GLState_glAlphaFunc : GLState
	{
		typedef std::unordered_map<std::string, EnumNamePair> CompareStateMap;

		GLState_glAlphaFunc(GLenum enumVal = GL_ALWAYS, GLclampf cref = 0) 
			: GLState(std::bind(::glAlphaFunc, enumVal, cref))
		{
			_stateMap["GL_NEVER"]		= EnumNamePair(GL_NEVER);
			_stateMap["GL_LESS"]		= EnumNamePair(GL_LESS);
			_stateMap["GL_EQUAL"]		= EnumNamePair(GL_EQUAL);
			_stateMap["GL_LEQUAL"]		= EnumNamePair(GL_LEQUAL);
			_stateMap["GL_GREATER"]		= EnumNamePair(GL_GREATER);
			_stateMap["GL_NOTEQUAL"]	= EnumNamePair(GL_NOTEQUAL);
			_stateMap["GL_GEQUAL"]		= EnumNamePair(GL_GEQUAL);
			_stateMap["GL_ALWAYS"]		= EnumNamePair(GL_ALWAYS);
		}

		virtual StateSetResetPair parse(const std::vector<std::string>& params) const
		{
			if(params.size() != 2)
				throw std::exception("Error: glAlphaFunc function takes two parameter.");

			std::string str = boost::to_upper_copy(boost::trim_copy(params[0]));
			float cref = parse_float(params[1]);

			CompareStateMap::const_iterator cItr = _stateMap.find(str);
			if(cItr == _stateMap.end())
				throw std::exception("Error: Invalid glAlphaFunc function state.");
			const EnumNamePair& enumNamePair = cItr->second;
			return StateSetResetPair(std::bind(::glAlphaFunc, enumNamePair.enumVal, cref), get_default_state_fn(), get_state_ID(), enumNamePair.enumName);
		}

	private:
		CompareStateMap _stateMap;
	};

	struct GLState_glDepthFunc : GLState
	{
		typedef std::unordered_map<std::string, EnumNamePair> CompareStateMap;

		GLState_glDepthFunc(GLenum enumVal = GL_ALWAYS) 
			: GLState(std::bind(::glDepthFunc, enumVal))
		{
			_stateMap["GL_NEVER"]		= EnumNamePair(GL_NEVER);
			_stateMap["GL_LESS"]		= EnumNamePair(GL_LESS);
			_stateMap["GL_EQUAL"]		= EnumNamePair(GL_EQUAL);
			_stateMap["GL_LEQUAL"]		= EnumNamePair(GL_LEQUAL);
			_stateMap["GL_GREATER"]		= EnumNamePair(GL_GREATER);
			_stateMap["GL_NOTEQUAL"]	= EnumNamePair(GL_NOTEQUAL);
			_stateMap["GL_GEQUAL"]		= EnumNamePair(GL_GEQUAL);
			_stateMap["GL_ALWAYS"]		= EnumNamePair(GL_ALWAYS);
		}

		virtual StateSetResetPair parse(const std::vector<std::string>& params) const
		{
			if(params.size() != 1)
				throw std::exception("Error: glDepthFunc function takes one parameter.");

			std::string str = boost::to_upper_copy(boost::trim_copy(params[0]));
			
			CompareStateMap::const_iterator cItr = _stateMap.find(str);
			if(cItr == _stateMap.end())
				throw std::exception("Error: Invalid glDepthFunc function state.");
			const EnumNamePair& enumNamePair = cItr->second;

			return StateSetResetPair(std::bind(::glDepthFunc, enumNamePair.enumVal), get_default_state_fn(), get_state_ID(), enumNamePair.enumName);
		}

	private:
		CompareStateMap _stateMap;
	};

	// states:
	struct GLState_glDepthMask : GLState
	{
		GLState_glDepthMask(GLboolean flag) 
			: GLState(std::bind(::glDepthMask, flag)), _flag(flag) {}

		virtual StateSetResetPair parse(const std::vector<std::string>& params) const
		{
			if(params.size() != 1)
				throw std::exception("Error: glDepthMask takes one parameter.");

			if(parse_bool(params[0]))
				return StateSetResetPair(std::bind(::glDepthMask, GL_TRUE), get_default_state_fn(), get_state_ID(), "true");
			else
				return StateSetResetPair(std::bind(::glDepthMask, GL_FALSE), get_default_state_fn(), get_state_ID(), "false");
		}

	private:
		GLboolean _flag;
	};

	struct GLState_glBlendFunc : GLState
	{
		typedef std::unordered_map<std::string, EnumNamePair> BlendFuncMap;

		GLState_glBlendFunc(GLenum sfactor = GL_ONE, GLenum dfactor = GL_ZERO) 
			: GLState(std::bind(::glBlendFunc, sfactor, dfactor))
		{
			_stateMap["GL_ZERO"]				= EnumNamePair(GL_ZERO);
			_stateMap["GL_ONE"]					= EnumNamePair(GL_ONE);
			_stateMap["GL_SRC_COLOR"]			= EnumNamePair(GL_SRC_COLOR);
			_stateMap["GL_ONE_MINUS_SRC_COLOR"] = EnumNamePair(GL_ONE_MINUS_SRC_COLOR);
			_stateMap["GL_DST_COLOR"]			= EnumNamePair(GL_DST_COLOR);
			_stateMap["GL_ONE_MINUS_DST_COLOR"] = EnumNamePair(GL_ONE_MINUS_DST_COLOR);
			_stateMap["GL_SRC_ALPHA"]			= EnumNamePair(GL_SRC_ALPHA);
			_stateMap["GL_ONE_MINUS_SRC_ALPHA"] = EnumNamePair(GL_ONE_MINUS_SRC_ALPHA);
			_stateMap["GL_DST_ALPHA"]			= EnumNamePair(GL_DST_ALPHA);
			_stateMap["GL_ONE_MINUS_DST_ALPHA"] = EnumNamePair(GL_ONE_MINUS_DST_ALPHA);
			_stateMap["GL_SRC_ALPHA_SATURATE"]	= EnumNamePair(GL_SRC_ALPHA_SATURATE);
		}

		virtual StateSetResetPair parse(const std::vector<std::string>& params) const
		{
			if(params.size() != 2)
				throw std::exception("Error: glBlendFunc function takes two parameters.");

			std::string sstr = boost::to_upper_copy(boost::trim_copy(params[0]));
			std::string dstr = boost::to_upper_copy(boost::trim_copy(params[1]));

			BlendFuncMap::const_iterator sItr = _stateMap.find(sstr);
			if(sItr == _stateMap.end())
				throw std::exception("Error: First glBlendFunc function state is invalid.");
			BlendFuncMap::const_iterator dItr = _stateMap.find(dstr);
			if(dItr == _stateMap.end())
				throw std::exception("Error: Second glBlendFunc function state is invalid.");

			const EnumNamePair& enumNamePair1 = sItr->second;
			const EnumNamePair& enumNamePair2 = dItr->second;

			return StateSetResetPair(std::bind(::glBlendFunc, enumNamePair1.enumVal, enumNamePair2.enumVal), get_default_state_fn(), get_state_ID(), enumNamePair1.enumName + "," + enumNamePair2.enumName);
		}

	private:
		BlendFuncMap _stateMap;
	};

	struct GLState_glBlendFuncSeparate : GLState
	{
		typedef std::unordered_map<std::string, EnumNamePair> BlendFuncMap;

		GLState_glBlendFuncSeparate(GLenum srcRGB = GL_ONE, GLenum dstRGB = GL_ZERO, GLenum srcAlpha = GL_ONE, GLenum dstAlpha = GL_ZERO) 
			: GLState(std::bind(::glBlendFuncSeparate, srcRGB, dstRGB, srcAlpha, dstAlpha))
		{
			_stateMap["GL_ZERO"]				= EnumNamePair(GL_ZERO);
			_stateMap["GL_ONE"]					= EnumNamePair(GL_ONE);
			_stateMap["GL_SRC_COLOR"]			= EnumNamePair(GL_SRC_COLOR);
			_stateMap["GL_ONE_MINUS_SRC_COLOR"] = EnumNamePair(GL_ONE_MINUS_SRC_COLOR);
			_stateMap["GL_DST_COLOR"]			= EnumNamePair(GL_DST_COLOR);
			_stateMap["GL_ONE_MINUS_DST_COLOR"] = EnumNamePair(GL_ONE_MINUS_DST_COLOR);
			_stateMap["GL_SRC_ALPHA"]			= EnumNamePair(GL_SRC_ALPHA);
			_stateMap["GL_ONE_MINUS_SRC_ALPHA"] = EnumNamePair(GL_ONE_MINUS_SRC_ALPHA);
			_stateMap["GL_DST_ALPHA"]			= EnumNamePair(GL_DST_ALPHA);
			_stateMap["GL_ONE_MINUS_DST_ALPHA"] = EnumNamePair(GL_ONE_MINUS_DST_ALPHA);
			_stateMap["GL_SRC_ALPHA_SATURATE"]	= EnumNamePair(GL_SRC_ALPHA_SATURATE);
		}

		virtual StateSetResetPair parse(const std::vector<std::string>& params) const
		{
			if(params.size() != 4)
				throw std::exception("Error: glBlendFuncSeparate function takes four parameters.");

			std::string srcRGBstr = boost::to_upper_copy(boost::trim_copy(params[0]));
			std::string dstRGBstr = boost::to_upper_copy(boost::trim_copy(params[1]));
			std::string srcAlphastr = boost::to_upper_copy(boost::trim_copy(params[2]));
			std::string dstAlphastr = boost::to_upper_copy(boost::trim_copy(params[3]));

			auto srcRGBItr = _stateMap.find(srcRGBstr);
			if(srcRGBItr == _stateMap.end())
				throw std::exception("Error: First glBlendFuncSeparate function state is invalid.");
			auto dstRGBItr = _stateMap.find(dstRGBstr);
			if(dstRGBItr == _stateMap.end())
				throw std::exception("Error: Second glBlendFuncSeparate function state is invalid.");
			auto srcAlphaItr = _stateMap.find(srcAlphastr);
			if(srcAlphaItr == _stateMap.end())
				throw std::exception("Error: Third glBlendFuncSeparate function state is invalid.");
			auto dstAlphaItr = _stateMap.find(dstAlphastr);
			if(dstAlphaItr == _stateMap.end())
				throw std::exception("Error: Fourth glBlendFuncSeparate function state is invalid.");

			return StateSetResetPair(std::bind(::glBlendFuncSeparate, srcRGBItr->second.enumVal, dstRGBItr->second.enumVal, 
				srcAlphaItr->second.enumVal, dstAlphaItr->second.enumVal), get_default_state_fn(), get_state_ID(),
				srcRGBItr->second.enumName + "," + dstRGBItr->second.enumName + "," + srcAlphaItr->second.enumName + "," + dstAlphaItr->second.enumName);
		}

	private:
		BlendFuncMap _stateMap;
	};

	struct GLState_glCullFace : GLState
	{
		typedef std::unordered_map<std::string, EnumNamePair> CullFuncMap;

		GLState_glCullFace(GLenum mode = GL_BACK) 
			: GLState(std::bind(::glCullFace, mode))
		{
			_stateMap["GL_FRONT"]			= EnumNamePair(GL_FRONT);
			_stateMap["GL_BACK"]			= EnumNamePair(GL_BACK);
			_stateMap["GL_FRONT_AND_BACK"]	= EnumNamePair(GL_FRONT_AND_BACK);
		}

		virtual StateSetResetPair parse(const std::vector<std::string>& params) const
		{
			if(params.size() != 1)
				throw std::exception("Error: glCullFace function takes one parameter.");

			std::string mode = boost::to_upper_copy(boost::trim_copy(params[0]));

			CullFuncMap::const_iterator mItr = _stateMap.find(mode);
			if(mItr == _stateMap.end())
				throw std::exception("Error: First glCullFace function state is invalid.");

			const EnumNamePair& enumNamePair = mItr->second;

			return StateSetResetPair(std::bind(::glCullFace, enumNamePair.enumVal), get_default_state_fn(), get_state_ID(), enumNamePair.enumName);
		}

	private:
		CullFuncMap _stateMap;
	};

	struct GLState_glDepthRange : GLState
	{
		GLState_glDepthRange(float zNear = 0, float zFar = 1) 
			: GLState(std::bind(::glDepthRange, zNear, zFar))
		{
		}

		virtual StateSetResetPair parse(const std::vector<std::string>& params) const
		{
			if(params.size() != 2)
				throw std::exception("Error: glDepthRange function takes two float parameter.");

			float zNear = parse_float(params[0]);
			float zFar = parse_float(params[1]);

			std::stringstream ss;
			ss << zNear << "," << zFar;
			return StateSetResetPair(std::bind(::glDepthRange, zNear, zFar), get_default_state_fn(), get_state_ID(), ss.str());
		}
	};

	// states:
	struct GLState_glColorMask : GLState
	{
		GLState_glColorMask(GLboolean rflag, GLboolean gflag, GLboolean bflag, GLboolean aflag) 
			: GLState(std::bind(::glColorMask, rflag, gflag, bflag, aflag)) {}

		virtual StateSetResetPair parse(const std::vector<std::string>& params) const
		{
			if(params.size() != 4)
				throw std::exception("Error: glColorMask takes four parameter.");

			bool flags[4];
			std::stringstream ss;
			for(size_t idx = 0; idx < params.size(); ++idx)
			{
				flags[idx] = parse_bool(params[idx]);
				if (idx > 0)
					ss << ", ";
				ss << flags[idx] ? "true" : "false";
			}
			
			return StateSetResetPair(std::bind(::glColorMask, flags[0], flags[1], flags[2], flags[3]), get_default_state_fn(), get_state_ID(), ss.str());
		}
	};

public:
	StateTypes() 
	{
		init_state_types();
	}

	// end states
	void init_state_types()
	{
		_nameStateMap["GL_ALPHA_TEST"]		= GLState::ptr(new GLState_Boolean		(GL_ALPHA_TEST, false));
		_nameStateMap["GL_BLEND"]			= GLState::ptr(new GLState_Boolean		(GL_BLEND, false));
		_nameStateMap["GL_CULL_FACE"]		= GLState::ptr(new GLState_Boolean		(GL_CULL_FACE, true));
		_nameStateMap["GL_DEPTH_TEST"]		= GLState::ptr(new GLState_Boolean		(GL_DEPTH_TEST, true));
		_nameStateMap["glAlphaFunc"]		= GLState::ptr(new GLState_glAlphaFunc	());
		_nameStateMap["glDepthFunc"]		= GLState::ptr(new GLState_glDepthFunc	());
		_nameStateMap["glDepthMask"]		= GLState::ptr(new GLState_glDepthMask	(true));
		_nameStateMap["glBlendFunc"]		= GLState::ptr(new GLState_glBlendFunc	());
		_nameStateMap["glBlendFuncSeparate"]= GLState::ptr(new GLState_glBlendFuncSeparate	());
		_nameStateMap["glColorMask"]		= GLState::ptr(new GLState_glColorMask	(true, true, true, true));
		_nameStateMap["glCullFace"]			= GLState::ptr(new GLState_glCullFace	());
		_nameStateMap["glDepthRange"]		= GLState::ptr(new GLState_glDepthRange	());
	}

	GLState::StateSetResetPair parse_state(const std::string& name, 
		const std::vector< std::string >& vals)
	{
		NameStateMap::const_iterator cItr = _nameStateMap.find(name);
		if(cItr != _nameStateMap.end())
		{
			return cItr->second->parse(vals);
		}

		return GLState::StateSetResetPair();
	}

private:
	static void apply_default_state(const NameStateMap::value_type& statePair)
	{
		(statePair.second->get_default_state_fn())();
	}

public:
	void apply_default_states()
	{
		std::for_each(_nameStateMap.begin(), _nameStateMap.end(), 
			apply_default_state);
	}

	static StateTypes* get_instance() 
	{
		if(!_instance)
			_instance.reset(new StateTypes());
		return _instance.get();
	}

private:
	static std::shared_ptr<StateTypes> _instance;
	NameStateMap _nameStateMap;
};

}

#endif // __EFFECT_GLSTATE_H__
