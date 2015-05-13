#include <boost/preprocessor.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/preprocessor/seq/for_each.hpp>
#include <boost/preprocessor/tuple/elem.hpp>
#include <boost/preprocessor/tuple/to_seq.hpp>
#include <boost/preprocessor/control/if.hpp>
#include <boost/preprocessor/repetition/repeat_from_to.hpp>
#include <silicium/to_unique.hpp>

namespace Si
{

}

#define SILICIUM_DETAIL_REMOVE_PARENTHESES(...) __VA_ARGS__

#define SILICIUM_DETAIL_MAKE_PARAMETER(z, n, array) BOOST_PP_COMMA_IF(n) BOOST_PP_ARRAY_ELEM(n, array) BOOST_PP_CAT(arg, n)
#define SILICIUM_DETAIL_MAKE_PARAMETERS(array) ( BOOST_PP_REPEAT(BOOST_PP_ARRAY_SIZE(array), SILICIUM_DETAIL_MAKE_PARAMETER, array) )

#define SILICIUM_DETAIL_MAKE_PURE_VIRTUAL_METHOD(r, data, elem) \
	virtual \
	BOOST_PP_TUPLE_ELEM(3, 2, elem) \
	BOOST_PP_TUPLE_ELEM(3, 0, elem) \
	SILICIUM_DETAIL_MAKE_PARAMETERS(BOOST_PP_TUPLE_ELEM(3, 1, elem)) \
	= 0;

#define SILICIUM_DETAIL_MAKE_INTERFACE(name, methods) struct name { \
	virtual ~name() {} \
	BOOST_PP_SEQ_FOR_EACH(SILICIUM_DETAIL_MAKE_PURE_VIRTUAL_METHOD, _, methods) \
};

#define SILICIUM_DETAIL_CAT_COMMA_IMPL(a, b) a, b
#define SILICIUM_DETAIL_CAT_COMMA(a, b) SILICIUM_DETAIL_CAT_COMMA_IMPL(a, b)

#define SILICIUM_DETAIL_ERASER_METHOD_ARGUMENT(z, n, text) , BOOST_PP_CAT(_, n)

#define SILICIUM_DETAIL_MAKE_ERASER_METHOD(r, data, elem) \
	virtual \
	BOOST_PP_TUPLE_ELEM(3, 2, elem) \
	BOOST_PP_TUPLE_ELEM(3, 0, elem) \
	SILICIUM_DETAIL_MAKE_PARAMETERS(BOOST_PP_TUPLE_ELEM(3, 1, elem)) \
	SILICIUM_OVERRIDE { \
		return original. BOOST_PP_TUPLE_ELEM(3, 0, elem) ( \
			BOOST_PP_ENUM_PARAMS(BOOST_PP_ARRAY_SIZE(BOOST_PP_TUPLE_ELEM(3, 1, elem)), arg) \
		); \
	}


#define SILICIUM_DETAIL_MAKE_ERASER(name, methods) template <class Original> struct name : interface { \
	Original original; \
	explicit name(Original original) : original(std::move(original)) {} \
	BOOST_PP_SEQ_FOR_EACH(SILICIUM_DETAIL_MAKE_ERASER_METHOD, _, methods) \
};

#define SILICIUM_TRAIT(name, methods) struct name { \
	SILICIUM_DETAIL_MAKE_INTERFACE(interface, methods) \
	SILICIUM_DETAIL_MAKE_ERASER(eraser, methods) \
	template <class Original> \
	static auto erase(Original &&original) { \
		return eraser<typename std::decay<Original>::type>{std::forward<Original>(original)}; \
	} \
};

typedef long element;

SILICIUM_TRAIT(
	Producer,
	((get, (0, ()), element))
)

struct test_producer
{
	element get()
	{
		return 42;
	}
};

BOOST_AUTO_TEST_CASE(trivial_trait)
{
	std::unique_ptr<Producer::interface> p = Si::to_unique(Producer::erase(test_producer{}));
	BOOST_REQUIRE(p);
	BOOST_CHECK_EQUAL(42, p->get());
}

template <class T>
SILICIUM_TRAIT(
	Container,
	((emplace_back, (1, (T)), void))
	((resize, (1, (size_t)), void))
	((resize, (2, (size_t, T const &)), void))
)

BOOST_AUTO_TEST_CASE(templatized_trait)
{
	auto container = Container<int>::erase(std::vector<int>{});
	container.emplace_back(123);
	{
		std::vector<int> const expected{123};
		BOOST_CHECK(expected == container.original);
	}
	container.resize(2);
	{
		std::vector<int> const expected{123, 0};
		BOOST_CHECK(expected == container.original);
	}
	container.resize(3, 7);
	{
		std::vector<int> const expected{123, 0, 7};
		BOOST_CHECK(expected == container.original);
	}
}
