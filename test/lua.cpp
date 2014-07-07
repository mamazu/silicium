#include <reactive/bridge.hpp>
#include <reactive/consume.hpp>
#include <boost/optional.hpp>
#include <boost/test/unit_test.hpp>
#include <lua.hpp>

namespace Si
{
	struct lua_deleter
	{
		void operator()(lua_State *L) const
		{
			lua_close(L);
		}
	};

	std::unique_ptr<lua_State, lua_deleter> open_lua()
	{
		auto L = std::unique_ptr<lua_State, lua_deleter>(luaL_newstate());
		if (!L)
		{
			throw std::bad_alloc();
		}
		return L;
	}

	typedef rx::observer<int> yield_destination;

	static int yield(lua_State *L)
	{
		yield_destination &dest = *static_cast<yield_destination *>(lua_touserdata(L, lua_upvalueindex(1)));
		int element = lua_tointeger(L, 1);
		dest.got_element(element);
		return lua_yield(L, 0);
	}

	BOOST_AUTO_TEST_CASE(lua)
	{
		auto L = open_lua();
		// src
		BOOST_REQUIRE_EQUAL(0, luaL_loadstring(L.get(), "return function (yield) yield(4) end"));
		// fn
		if (0 != lua_pcall(L.get(), 0, 1, 0))
		{
			throw std::runtime_error(lua_tostring(L.get(), -1));
		}

		lua_State * const coro = lua_newthread(L.get());
		lua_xmove(L.get(), coro, 1);

		rx::bridge<int> yielded;
		// fn &yielded
		lua_pushlightuserdata(coro, &static_cast<yield_destination &>(yielded));
		// fn yield[&yielded]
		lua_pushcclosure(coro, yield, 1);

		boost::optional<int> got;
		auto consumer = rx::consume<int>([&got](boost::optional<int> element)
		{
			BOOST_REQUIRE(element);
			got = element;
		});

		yielded.async_get_one(consumer);
		if (LUA_YIELD != lua_resume(L.get(), 1))
		{
			throw std::runtime_error(lua_tostring(coro, -1));
		}
		BOOST_CHECK_EQUAL(boost::make_optional(4), got);
	}
}
