#include "context.h"

namespace vice {;

//v8::Isolate* gIsolate = NULL;
//
//std::shared_ptr<void> Context::static_init()
//{
//	gIsolate = v8::Isolate::New();
//	gIsolate->Enter();
//
//	return std::shared_ptr<void>(nullptr, [](void*) { Context::static_release(); });
//}
//
//void Context::static_release()
//{
//	gIsolate->Exit();
//	gIsolate->Dispose();
//}
//
//Context::Context()
//	: _v8Context(v8::Context::New(gIsolate))
//{
//
//}

}
