#ifndef SILICIUM_COROUTINE_HPP
#define SILICIUM_COROUTINE_HPP

#include <silicium/exchange.hpp>
#include <silicium/observable/observer.hpp>
#include <silicium/config.hpp>
#include <silicium/observable/yield_context.hpp>
#include <boost/coroutine/all.hpp>

namespace Si
{
	namespace detail
	{
		struct coroutine_yield_context : basic_yield_context
		{
#if BOOST_VERSION >= 105500
			typedef boost::coroutines::coroutine<observable<nothing> *>::push_type consumer_type;
#else
			typedef boost::coroutines::coroutine<observable<nothing> *()>::caller_type consumer_type;
#endif

			explicit coroutine_yield_context(consumer_type &consumer)
				: consumer(&consumer)
			{
			}

			virtual void get_one(observable<nothing> &target) SILICIUM_OVERRIDE
			{
				(*consumer)(&target);
			}

		private:

			consumer_type *consumer;
		};
	}

	template <class Element>
	struct coroutine_observable : private observer<nothing>
	{
		typedef Element element_type;

		coroutine_observable()
			: receiver_(nullptr)
			, has_finished(false)
		{
		}

		coroutine_observable(coroutine_observable &&other)
			: coro_(std::move(other.coro_))
			, action(std::move(other.action))
			, receiver_(nullptr)
			, has_finished(false)
		{
		}

		template <class Action>
		explicit coroutine_observable(Action &&action)
			: coro_()
			, action(std::forward<Action>(action))
			, receiver_(nullptr)
			, has_finished(false)
		{
		}

		coroutine_observable &operator = (coroutine_observable &&other)
		{
			coro_ = std::move(other.coro_);
			action = std::move(other.action);
			receiver_ = std::move(other.receiver_);
			has_finished = other.has_finished;
			return *this;
		}

		void async_get_one(Si::observer<element_type> &receiver)
		{
			receiver_ = &receiver;
			next();
		}

	private:

		typedef Si::observable<nothing> *command_type;
		typedef
#if BOOST_VERSION >= 105500
			typename boost::coroutines::coroutine<command_type>::pull_type
#else
			boost::coroutines::coroutine<command_type ()>
#endif
		coroutine_type;
		typedef std::shared_ptr<coroutine_type> coroutine_holder;

		coroutine_holder coro_;
		std::function<Element (yield_context)> action;
		Si::observer<Element> *receiver_;
		bool has_finished;
		
		virtual void got_element(nothing) SILICIUM_OVERRIDE
		{
			next();
		}

		virtual void ended() SILICIUM_OVERRIDE
		{
			next();
		}

		void next()
		{
			auto keep_coro_alive = coro_;
			if (has_finished)
			{
				return Si::exchange(receiver_, nullptr)->ended();
			}
			if (action)
			{
				auto bound_action = Si::exchange(action, decltype(action)());
				coro_ =
					std::make_shared<coroutine_type>
						([bound_action, this](
#if BOOST_VERSION >= 105500
							typename boost::coroutines::coroutine<command_type>::push_type
#else
							typename coroutine_type::caller_type
#endif
							&push)
						{
							detail::coroutine_yield_context yield_impl(push);
							yield_context yield(yield_impl); //TODO: save this indirection
							auto result = bound_action(yield);
							assert(!has_finished);
							has_finished = true;
							Si::exchange(receiver_, nullptr)->got_element(std::move(result));
						});
			}
			else if (*coro_)
			{
				(*coro_)();
			}
			if (!receiver_)
			{
				return;
			}
			if (*coro_)
			{
				command_type command = coro_->get();
				command->async_get_one(*this);
			}
			else
			{
				Si::exchange(receiver_, nullptr)->ended();
			}
		}

		SILICIUM_DELETED_FUNCTION(coroutine_observable(coroutine_observable const &))
		SILICIUM_DELETED_FUNCTION(coroutine_observable &operator = (coroutine_observable const &))
	};

	template <class Action, class Element = decltype(std::declval<Action>()(std::declval<yield_context>()))>
	auto make_coroutine(Action &&action)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
		-> coroutine_observable<Element>
#endif
	{
		return coroutine_observable<Element>(std::forward<Action>(action));
	}
}

#endif
