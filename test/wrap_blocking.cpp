#include <silicium/coroutine.hpp>
#include <silicium/consume.hpp>
#include <boost/test/unit_test.hpp>
#include <boost/thread/future.hpp>
#include <boost/asio/io_service.hpp>

namespace Si
{
	template <class Observable>
	struct has_blocking_get_one : std::false_type
	{
	};

	template <class Action>
	struct blocking_observable
	{
		using element_type = decltype(std::declval<Action>()());

		explicit blocking_observable(Action act)
			: act(std::move(act))
		{
		}

#if !SILICIUM_COMPILER_GENERATES_MOVES
		blocking_observable(blocking_observable &&other)
			: act(std::move(other.act))
			, done(std::move(other.done))
		{
		}

		blocking_observable &operator = (blocking_observable &&other)
		{
			act = std::move(other.act);
			done = std::move(other.done);
			return *this;
		}
#endif

		void async_get_one(observer<element_type> &receiver)
		{
			done = boost::async(boost::launch::async, [this, &receiver]()
			{
				element_type result = act();
				receiver.got_element(std::move(result));
			});
		}

		void cancel()
		{
			throw std::logic_error("to do");
		}

		element_type get_one()
		{
			return act();
		}

	private:

		Action act;
		boost::unique_future<void> done;
	};

	template <class Action>
	struct has_blocking_get_one<blocking_observable<Action>> : std::true_type
	{
	};

	template <class Action>
	auto wrap_blocking(Action &&act) -> blocking_observable<typename std::decay<Action>::type>
	{
		return blocking_observable<typename std::decay<Action>::type>(act);
	}
}

namespace
{
	int blocking_stuff()
	{
		return 2;
	}
}

BOOST_AUTO_TEST_CASE(wrap_blocking_coroutine)
{
	auto coro = Si::make_coroutine<int>([](Si::yield_context<int> &yield)
	{
		auto blocking = Si::wrap_blocking(blocking_stuff);
		auto const intermediate_result = yield.get_one(blocking);
		BOOST_REQUIRE(intermediate_result);
		yield(*intermediate_result + 3);
	});
	boost::asio::io_service io;
	boost::optional<boost::asio::io_service::work> work(boost::in_place(boost::ref(io)));
	auto consumer = Si::consume<int>([&work](int element)
	{
		BOOST_CHECK_EQUAL(5, element);
		work = boost::none;
	});
	coro.async_get_one(consumer);
	io.run();
}