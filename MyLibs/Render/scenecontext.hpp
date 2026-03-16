#ifndef _SCENE_SCENECONTEXT_H
#define _SCENE_SCENECONTEXT_H


#include <vector>
#include "renderstage.hpp"
#include "Scene/transform.hpp"
#include "Scene/light.hpp"
#include "Scene/camera.hpp"
#include "Scene/coordtraits.hpp"
#include "Scene/geometry.hpp"

namespace render {;

struct SceneContext
{

	typedef std::shared_ptr< SceneContext > ptr;

	typedef std::set<RenderStage::ptr> RenderStageList;
	typedef RenderStageList::iterator RenderStageIterator;
	typedef RenderStageList::const_iterator ConstRenderStageIterator;

private:
	RenderStageList _stages;

	float _frameTime;

public:
	RenderStageIterator beginStages() { return _stages.begin(); }
	RenderStageIterator endStages() { return _stages.end(); }
	ConstRenderStageIterator beginStages() const { return _stages.begin(); }
	ConstRenderStageIterator endStages() const { return _stages.end(); }
	RenderStageList::size_type stagesCount() const { return _stages.size(); }
	void addStage(RenderStage::ptr stage) { _stages.insert(stage); }

	template < class T > 
	void setFrameTime(T ftime) { _frameTime = ftime; }

	float frameTime() const { return _frameTime; }

	template < class Pred > 
	RenderStageIterator findRenderStage(Pred predicate)
	{
		return std::find_if(_stages.begin(), _stages.end(), predicate);
	}

	template < class Pred > 
	ConstRenderStageIterator findRenderStage(Pred predicate) const
	{
		return std::find_if(_stages.begin(), _stages.end(), predicate);
	}
};

}

#endif // _SCENE_SCENECONTEXT_H