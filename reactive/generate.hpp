#ifndef SILICIUM_REACTIVE_GENERATE_HPP
#define SILICIUM_REACTIVE_GENERATE_HPP

#include <reactive/observable.hpp>

namespace rx
{
	template <class Generated, class Element = typename std::result_of<Generated ()>::type>
	struct generator : observable<Element>
	{
		explicit generator(Generated generate)
			: generate(std::move(generate))
		{
		}

		virtual void async_get_one(observer<Element> &receiver) SILICIUM_OVERRIDE
		{
			return receiver.got_element(generate());
		}

		virtual void cancel() SILICIUM_OVERRIDE
		{
		}

	private:

		Generated generate;
	};

	template <class Generated>
	auto generate(Generated &&generate)
	{
		return generator<typename std::decay<Generated>::type>(std::forward<Generated>(generate));
	}
}

#endif
