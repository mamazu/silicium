#ifndef SILICIUM_REACTIVE_WHILE_HPP
#define SILICIUM_REACTIVE_WHILE_HPP

#include <silicium/observable/observer.hpp>
#include <silicium/config.hpp>
#include <utility>

namespace Si
{
	template <class Input, class ElementPredicate>
	struct while_observable
		: private observer<typename Input::element_type>
	{
		typedef typename Input::element_type element_type;

		while_observable(Input input, ElementPredicate is_not_end)
			: input(std::move(input))
			, is_not_end(std::move(is_not_end))
			, receiver_(nullptr)
		{
		}

		template <class Observer>
		void async_get_one(Observer &&receiver)
		{
			assert(!receiver_);
			receiver_ = receiver.get();
			input.async_get_one(extend(std::forward<Observer>(receiver), observe_by_ref(static_cast<observer<typename Input::element_type> &>(*this))));
		}

	private:

		Input input;
		ElementPredicate is_not_end;
		observer<element_type> *receiver_;

		virtual void got_element(typename Input::element_type value) SILICIUM_OVERRIDE
		{
			assert(receiver_);
			if (is_not_end(value))
			{
				exchange(receiver_, nullptr)->got_element(std::move(value));
			}
			else
			{
				exchange(receiver_, nullptr)->ended();
			}
		}

		virtual void ended() SILICIUM_OVERRIDE
		{
			assert(receiver_);
			exchange(receiver_, nullptr)->ended();
		}
	};

	template <class Input, class ElementPredicate>
	auto while_(Input &&input, ElementPredicate &&is_not_end) -> while_observable<
		typename std::decay<Input>::type,
		typename std::decay<ElementPredicate>::type>
	{
		return while_observable<
				typename std::decay<Input>::type,
				typename std::decay<ElementPredicate>::type>(std::forward<Input>(input), std::forward<ElementPredicate>(is_not_end));
	}
}

#endif
