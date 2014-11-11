#include <silicium/source/enumerating_source.hpp>
#include <silicium/source/memory_source.hpp>
#include <silicium/source/single_source.hpp>
#include <boost/test/unit_test.hpp>

namespace Si
{
	template <class Element>
	struct empty_source : source<Element>
	{
		typedef Element element_type;

		virtual boost::iterator_range<element_type const *> map_next(std::size_t size) SILICIUM_OVERRIDE
		{
			boost::ignore_unused_variable_warning(size);
			return {};
		}

		virtual element_type *copy_next(boost::iterator_range<element_type *> destination) SILICIUM_OVERRIDE
		{
			boost::ignore_unused_variable_warning(destination);
			return destination.begin();
		}
	};
}

BOOST_AUTO_TEST_CASE(enumerating_source_directly_empty)
{
	auto s = Si::make_enumerating_source(Si::empty_source<boost::iterator_range<int *>>());
	boost::optional<int> e = Si::get(s);
	BOOST_CHECK(!e);
}

BOOST_AUTO_TEST_CASE(enumerating_source_indirectly_empty)
{
	auto s = Si::make_enumerating_source(Si::make_single_source(boost::iterator_range<int *>{}));
	boost::optional<int> e = Si::get(s);
	BOOST_CHECK(!e);
}

BOOST_AUTO_TEST_CASE(enumerating_source_non_empty)
{
	int const element = 3;
	auto s = Si::make_enumerating_source(Si::make_single_source(boost::iterator_range<int const *>{&element, &element + 1}));
	boost::optional<int> e = Si::get(s);
	BOOST_REQUIRE(e);
	BOOST_CHECK_EQUAL(boost::make_optional(3), e);
}

BOOST_AUTO_TEST_CASE(enumerating_source_map_next_at_the_end)
{
	int const element = 3;
	auto s = Si::make_enumerating_source(Si::make_single_source(boost::iterator_range<int const *>{&element, &element + 1}));
	boost::iterator_range<int const *> m = s.map_next(2);
	BOOST_CHECK_EQUAL(&element, m.begin());
	BOOST_CHECK_EQUAL(1, m.size());
	BOOST_CHECK(!Si::get(s));
}