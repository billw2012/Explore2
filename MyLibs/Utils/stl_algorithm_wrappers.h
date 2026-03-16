
#include <algorithm>

namespace std {;

template < typename Cont_, typename Pred_>
typename Cont_::const_iterator find_if(const Cont_& cont, Pred_ pred)
{
	return find_if(std::begin(cont), std::end(end), pred);
}

template < typename Cont_, typename Pred_>
typename Cont_::iterator find_if(Cont_& cont, Pred_ pred)
{
	return find_if(std::begin(cont), std::end(end), pred);
}

template < typename Cont_, typename Val_>
typename Cont_::const_iterator find(const Cont_& cont, Val_ val)
{
	return find(std::begin(cont), std::end(end), pred);
}

template < typename Cont_, typename Val_>
typename Cont_::iterator find(Cont_& cont, Val_ val)
{
	return find(std::begin(cont), std::end(end), val);
}

}