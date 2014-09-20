#ifndef SILICIUM_SINGLE_SOURCE_HPP
#define SILICIUM_SINGLE_SOURCE_HPP

#include <silicium/config.hpp>
#include <boost/range/iterator_range.hpp>

namespace Si
{
	template <class Element>
	struct single_source
	{
		using element_type = Element;

		single_source()
		{
		}

		explicit single_source(Element element)
			: element(std::move(element))
		{
		}

		boost::iterator_range<element_type const *> map_next(std::size_t size)
		{
			boost::ignore_unused_variable_warning(size);
			if (used)
			{
				return {};
			}
			used = true;
			return boost::make_iterator_range(&element, &element + 1);
		}

		element_type *copy_next(boost::iterator_range<element_type *> destination)
		{
			if (used || destination.empty())
			{
				return destination.begin();
			}
			*destination.begin() = std::move(element);
			used = true;
			return destination.begin() + 1;
		}

	private:

		Element element;
		bool used = false;
	};

	template <class Element>
	auto make_single_source(Element &&element)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
		-> single_source<typename std::decay<Element>::type>
#endif
	{
		return single_source<typename std::decay<Element>::type>(std::forward<Element>(element));
	}
}

#endif
