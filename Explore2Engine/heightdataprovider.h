#if !defined(__EXPLORE2_HEIGHT_DATA_PROVIDER_H__)
#define __EXPLORE2_HEIGHT_DATA_PROVIDER_H__

#include <iostream>
#include "Math/vector3.hpp"
#include "Math/ridgedmultifractal.hpp"

#include "HPCNoise/perlin.hpp"

namespace explore2 {;

struct HeightDataProvider
{
	typedef std::shared_ptr< HeightDataProvider > ptr;

	typedef float value_type;

	typedef math::Vector3<value_type> Vector3type;

	typedef HPCNoise::Noise<value_type> NoiseGen;
	typedef NoiseGen::vector4_type vector_type;
	typedef NoiseGen::pts_vector_type pts_vector_type;
	typedef NoiseGen::results_vector_type results_vector_type;
private:
	static const value_type Lacunarity;
	static const value_type Octaves;
	static const value_type H;
	static const value_type Offset;
	static const value_type Gain;

	NoiseGen _noise;

	value_type _scale, _vscale;

public:
	HeightDataProvider(value_type scale_ = 1, value_type vscale = 1, 
		value_type octaves = Octaves,  unsigned short seed = 0, value_type lacunarity = Lacunarity, 
		value_type h = H, value_type offset = Offset, value_type gain = Gain) 
		: _scale(scale_), 
		_vscale(vscale) 
	{
		_noise.add_octave(NoiseGen::OctavePtr(new NoiseGen::BasicOctave(static_cast<NoiseGen::real_type>(0.4), lacunarity)), 16);
		_noise.init(seed);
	}

	//virtual value_type get(const Vector3type& vec, bool scale = true) const
	//{
	//	return (value_type)0.0;
	//	//pts_vector_type pts;
	//	//pts.push_back(vector_type(vec.x, vec.y, vec.z));
	//	//return get(pts, scale)[0];


	//	//if(scale) return _fractalHigh(vec * -_vscale) * _scale;
	//	//else return _fractalHigh(vec * -_vscale);
	//}

	virtual results_vector_type get(const pts_vector_type& pts, bool scale = true)
	{
		NoiseGen::NoiseExecutionPtr exec = _noise.enqueue_work(pts, scale? _scale : (NoiseGen::real_type)1);
		return exec->get_results();
	}

	virtual results_vector_type get(const vector_type* pts, size_t ptsCount, bool scale = true)
	{
		NoiseGen::NoiseExecutionPtr exec = _noise.enqueue_work(pts, ptsCount, scale? _scale : (NoiseGen::real_type)1);
		return exec->get_results();
	}

	virtual NoiseGen::NoiseExecutionPtr queue(const pts_vector_type& pts, bool scale = true)
	{
		return _noise.enqueue_work(pts, scale? _scale : (NoiseGen::real_type)1);
	}

	value_type scale() const { return _scale; }

private:
	template < class FTy_ >
	void scaleVal(FTy_& val) const
	{
		val *= (FTy_)_scale;
	}
};

}

#endif // __EXPLORE2_HEIGHT_DATA_PROVIDER_H__