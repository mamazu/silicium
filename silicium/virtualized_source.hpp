#ifndef SILICIUM_VIRTUALIZED_SOURCE_HPP
#define SILICIUM_VIRTUALIZED_SOURCE_HPP

#include <silicium/source.hpp>
#include <silicium/override.hpp>
#include <boost/concept/requires.hpp>

namespace Si
{
	template <class Original>
	struct virtualized_source : source<typename Original::element_type>
	{
		using element_type = typename Original::element_type;

		virtualized_source()
		{
		}

		explicit virtualized_source(Original original)
			: original(std::move(original))
		{
		}

		virtual boost::iterator_range<element_type const *> map_next(std::size_t size) SILICIUM_OVERRIDE
		{
			return original.map_next(size);
		}

		virtual element_type *copy_next(boost::iterator_range<element_type *> destination) SILICIUM_OVERRIDE
		{
			return original.copy_next(destination);
		}

		virtual boost::uintmax_t minimum_size() SILICIUM_OVERRIDE
		{
			return original.minimum_size();
		}

		virtual boost::optional<boost::uintmax_t> maximum_size() SILICIUM_OVERRIDE
		{
			return original.maximum_size();
		}

		virtual std::size_t skip(std::size_t count) SILICIUM_OVERRIDE
		{
			return original.skip(count);
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