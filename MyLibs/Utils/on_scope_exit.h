#pragma once

#include <functional>

#define on_scope_exit(_op_) _on_scope_exit _scoped_var##__LINE__([&]() _op_);

struct _on_scope_exit
{
	_on_scope_exit(const std::function<void()>& fn_) : fn(fn_) {}
	~_on_scope_exit() { fn(); }

private:
	std::function<void()> fn;
};