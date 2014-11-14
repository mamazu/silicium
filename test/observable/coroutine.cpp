#include <silicium/observable/coroutine.hpp>
#include <silicium/observable/consume.hpp>
#include <silicium/observable/bridge.hpp>
#include <silicium/observable/total_consumer.hpp>
#include <silicium/observable/for_each.hpp>
#include <silicium/to_unique.hpp>
#include <boost/test/unit_test.hpp>

BOOST_AUTO_TEST_CASE(coroutine_trivial)
{
	auto coro = Si::make_coroutine([](Si::yield_context)
	{
		return 2;
	});
	bool got_element = false;
	auto consumer_ = Si::consume<int>([&got_element](int i)
	{
		BOOST_CHECK_EQUAL(2, i);
		got_element = true;
	});
	coro.async_get_one(consumer_);
	BOOST_CHECK(got_element);
}

BOOST_AUTO_TEST_CASE(coroutine_yield)
{
	Si::bridge<int> e;
	auto coro = Si::make_coroutine([&e](Si::yield_context yield)
	{
		return *yield.get_one(e) + 1;
	});
	bool got_element = false;
	auto consumer_ = Si::consume<int>([&got_element](int i)
	{
		BOOST_CHECK_EQUAL(5, i);
		got_element = true;
	});
	coro.async_get_one(consumer_);
	BOOST_CHECK(!got_element);
	e.got_element(4);
	BOOST_CHECK(got_element);
}

BOOST_AUTO_TEST_CASE(coroutine_total_consumer)
{
	bool executed = false;
	auto consumer = Si::make_total_consumer(Si::make_coroutine([&executed](Si::yield_context)
	{
		executed = true;
		return Si::nothing();
	}));
	BOOST_CHECK(!executed);
	consumer.start();
	BOOST_CHECK(executed);
}

BOOST_AUTO_TEST_CASE(coroutine_self_destruct)
{
	std::size_t steps_done = 0;
	auto coro = Si::to_unique(Si::make_coroutine([&steps_done](Si::yield_context) -> Si::nothing
	{
		BOOST_REQUIRE_EQUAL(1, steps_done);
		++steps_done;
		return {};
	}));
	BOOST_REQUIRE_EQUAL(0, steps_done);
	auto handler = Si::for_each(Si::ref(*coro), [&coro, &steps_done](Si::nothing)
	{
		//this function is called in the coroutine
		BOOST_REQUIRE_EQUAL(2, steps_done);
		++steps_done;
		//destroying the coroutine itself now should not crash or anything, it just works.
		coro.reset();
	});
	BOOST_REQUIRE_EQUAL(0, steps_done);
	++steps_done;
	handler.start();
	BOOST_CHECK_EQUAL(3, steps_done);
	BOOST_CHECK(!coro);
}