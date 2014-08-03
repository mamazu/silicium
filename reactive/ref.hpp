#ifndef SILICIUM_REACTIVE_REF_HPP
#define SILICIUM_REACTIVE_REF_HPP

#include <reactive/ptr_observable.hpp>

namespace rx
{
	template <class Element>
	using reference = ptr_observable<Element, observable<Element> *>;

	template <class Element>
	auto ref(observable<Element> &identity) -> reference<Element>
	{
		return reference<Element>(&identity);
	}
}

#endif
