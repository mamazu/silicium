#ifndef SILICIUM_REACTIVE_REF_HPP
#define SILICIUM_REACTIVE_REF_HPP

#include <silicium/ptr_observable.hpp>

namespace Si
{
	template <class Observable, class Element = typename Observable::element_type>
	auto ref(Observable &identity) -> ptr_observable<Element, Observable *>
	{
		return ptr_observable<Element, Observable *>(&identity);
	}
}

#endif
