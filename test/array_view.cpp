#include <silicium/array_view.hpp>
#include <boost/test/unit_test.hpp>

// a fixed-size array_view should only contain one pointer
BOOST_STATIC_ASSERT(sizeof(int *) == sizeof(Si::array_view<int, Si::bounded_int<std::size_t, 1, 1>>));

BOOST_STATIC_ASSERT((2 * sizeof(int *)) == sizeof(Si::array_view<int, Si::bounded_int<std::size_t, 1, 2>>));

BOOST_AUTO_TEST_CASE(array_view_default_constructor)
{
	{
		Si::array_view<int> arr;
		BOOST_CHECK(arr.empty());
		BOOST_CHECK_EQUAL(static_cast<int *>(nullptr), arr.data());
		BOOST_CHECK_EQUAL(0, arr.length().value());
	}
	{
		Si::array_view<int, Si::bounded_int<std::size_t, 0, 100>> arr;
		BOOST_CHECK(arr.empty());
		BOOST_CHECK_EQUAL(static_cast<int *>(nullptr), arr.data());
		BOOST_CHECK_EQUAL(0, arr.length().value());
	}
}

BOOST_AUTO_TEST_CASE(array_view_from_std_array)
{
	std::array<int, 1> a1;
	std::array<int, 2> a2;
	std::array<int, 3> a3;
	Si::array_view<int, Si::bounded_int<std::size_t, 0, 3>> view;
	BOOST_CHECK(view.empty());

	view = a1;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(1, view.length().value());
	BOOST_CHECK_EQUAL(a1.data(), view.to_range().begin());

	view = a2;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(2, view.length().value());
	BOOST_CHECK_EQUAL(a2.data(), view.to_range().begin());

	view = a3;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(3, view.length().value());
	BOOST_CHECK_EQUAL(a3.data(), view.to_range().begin());
}

BOOST_AUTO_TEST_CASE(array_view_from_const_std_array)
{
	std::array<int, 1> const a1 = {{}};
	std::array<int, 2> const a2 = {{}};
	std::array<int, 3> const a3 = {{}};
	Si::array_view<int const, Si::bounded_int<std::size_t, 0, 3>> view;
	BOOST_CHECK(view.empty());

	view = a1;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(1, view.length().value());
	BOOST_CHECK_EQUAL(a1.data(), view.to_range().begin());

	view = a2;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(2, view.length().value());
	BOOST_CHECK_EQUAL(a2.data(), view.to_range().begin());

	view = a3;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(3, view.length().value());
	BOOST_CHECK_EQUAL(a3.data(), view.to_range().begin());
}

BOOST_AUTO_TEST_CASE(array_view_from_boost_array)
{
	boost::array<int, 1> a1;
	boost::array<int, 2> a2;
	boost::array<int, 3> a3;
	Si::array_view<int, Si::bounded_int<std::size_t, 0, 3>> view;
	BOOST_CHECK(view.empty());

	view = a1;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(1, view.length().value());
	BOOST_CHECK_EQUAL(a1.data(), view.to_range().begin());

	view = a2;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(2, view.length().value());
	BOOST_CHECK_EQUAL(a2.data(), view.to_range().begin());

	view = a3;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(3, view.length().value());
	BOOST_CHECK_EQUAL(a3.data(), view.to_range().begin());
}

BOOST_AUTO_TEST_CASE(array_view_from_const_boost_array)
{
	boost::array<int, 1> const a1 = {{}};
	boost::array<int, 2> const a2 = {{}};
	boost::array<int, 3> const a3 = {{}};
	Si::array_view<int const, Si::bounded_int<std::size_t, 0, 3>> view;
	BOOST_CHECK(view.empty());

	view = a1;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(1, view.length().value());
	BOOST_CHECK_EQUAL(a1.data(), view.to_range().begin());

	view = a2;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(2, view.length().value());
	BOOST_CHECK_EQUAL(a2.data(), view.to_range().begin());

	view = a3;
	BOOST_CHECK(!view.empty());
	BOOST_CHECK_EQUAL(3, view.length().value());
	BOOST_CHECK_EQUAL(a3.data(), view.to_range().begin());
}

BOOST_AUTO_TEST_CASE(array_view_default_length_type)
{
	Si::array_view<int> v;
	BOOST_CHECK(v.empty());
	{
		std::array<int, 3> arr;
		v = arr;
		BOOST_CHECK_EQUAL(arr.data(), v.to_range().begin());
		BOOST_CHECK_EQUAL(3, v.length().value());
	}
	{
		std::vector<int> vec(5);
		v = vec;
		BOOST_CHECK_EQUAL(vec.data(), v.to_range().begin());
		BOOST_CHECK_EQUAL(5, v.length().value());
	}
}

BOOST_AUTO_TEST_CASE(array_view_default_length_type_const)
{
	Si::array_view<int const> v;
	BOOST_CHECK(v.empty());
	{
		std::array<int, 3> const arr = {{}};
		v = arr;
		BOOST_CHECK_EQUAL(arr.data(), v.to_range().begin());
		BOOST_CHECK_EQUAL(3, v.length().value());
	}
	{
		std::vector<int> const vec(5);
		v = vec;
		BOOST_CHECK_EQUAL(vec.data(), v.to_range().begin());
		BOOST_CHECK_EQUAL(5, v.length().value());
	}
}

BOOST_AUTO_TEST_CASE(array_view_begin_end)
{
	std::array<int, 3> const arr = {{1, 2, 3}};
	Si::array_view<int const> view = arr;
	BOOST_CHECK_EQUAL_COLLECTIONS(arr.begin(), arr.end(), view.begin(), view.end());
	int sum = 0;
	for (int e : view)
	{
		sum += e;
	}
	BOOST_CHECK_EQUAL(6, sum);
}

BOOST_AUTO_TEST_CASE(array_view_index_operator)
{
	std::array<int, 3> const arr = {{1, 2, -3}};
	typedef Si::bounded_int<std::size_t, 3, 3> length;
	typedef Si::bounded_int<std::size_t, 0, 2> index;
	Si::array_view<int const, length> const view = arr;
	BOOST_CHECK_EQUAL(1, view[index::literal<0>()]);
	BOOST_CHECK_EQUAL(2, view[index::literal<1>()]);
	BOOST_CHECK_EQUAL(-3, view[index::literal<2>()]);
}
