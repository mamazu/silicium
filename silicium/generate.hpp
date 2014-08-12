#ifndef SILICIUM_REACTIVE_GENERATE_HPP
#define SILICIUM_REACTIVE_GENERATE_HPP

#include <silicium/config.hpp>
#include <silicium/override.hpp>
#include <silicium/observer.hpp>

namespace Si
{
	template <class Generated, class Element = typename std::result_of<Generated ()>::type>
	struct generator
	{
		using element_type = Element;

		generator()
		{
		}

		explicit generator(Generated generate)
			: generate(std::move(generate))
		{
		}

#if !SILICIUM_COMPILER_GENERATES_MOVES
		generator(generator &&other)
			: generate(std::move(other.generate))
		{
		}

		generator &operator = (generator &&other)
		{
			generate = std::move(other.generate);
			return *this;
		}
#endif

		void async_get_one(observer<Element> &receiver)
		{
			return receiver.got_element(generate());
		}

		void cancel()
		{
		}

	private:

		Generated generate;
	};

	template <class Generated>
	auto generate(Generated &&generate) -> generator<typename std::decay<Generated>::type>
	{
		return generator<typename std::decay<Generated>::type>(std::forward<Generated>(generate));
	}
}

#endif
