#ifndef SILICIUM_VIRTUALIZED_SOURCE_HPP
#define SILICIUM_VIRTUALIZED_SOURCE_HPP

#include <silicium/source/source.hpp>
#include <silicium/config.hpp>
#include <boost/concept/requires.hpp>

namespace Si
{
	template <class Original>
	struct virtualized_source : source<typename Original::element_type>
	{
		typedef typename Original::element_type element_type;

		virtualized_source()
		{
		}

		explicit virtualized_source(Original original)
			: original(std::move(original))
		{
		}

		virtual iterator_range<element_type const *> map_next(std::size_t size) SILICIUM_OVERRIDE
		{
			return original.map_next(size);
		}

		virtual element_type *copy_next(iterator_range<element_type *> destination) SILICIUM_OVERRIDE
		{
			return original.copy_next(destination);
		}

	private:

		Original original;
	};

	template <class Source>
	BOOST_CONCEPT_REQUIRES(
		((Si::Source<typename std::decay<Source>::type>)),
		(virtualized_source<typename std::decay<Source>::type>))
	virtualize_source(Source &&next)
	{
		return virtualized_source<typename std::decay<Source>::type>(std::forward<Source>(next));
	}
}

#endif
