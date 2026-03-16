#pragma once

#include <vector>

template < class Cont_, class Ty_ >
void unique_push(Cont_& cont, const Ty_& val)
{
	auto itr = std::find(cont.begin(), cont.end(), val);
	if(itr == cont.end())
		cont.push_back(val);
}

template < class Cont_, class Ty_ >
void unique_erase(Cont_& cont, const Ty_& val)
{
	auto itr = std::find(cont.begin(), cont.end(), val);
	if(itr != cont.end())
		cont.erase(itr);
}