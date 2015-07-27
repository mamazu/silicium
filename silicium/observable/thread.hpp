#ifndef SILICIUM_THREAD_HPP
#define SILICIUM_THREAD_HPP

#include <silicium/optional.hpp>
#include <functional>

#define SILICIUM_HAS_THREAD_OBSERVABLE SILICIUM_HAS_EXCEPTIONS
namespace Si
{
#if SILICIUM_HAS_THREAD_OBSERVABLE
	template <class SuccessElement, class ThreadingAPI>
	struct thread_observable
	{
		typedef typename ThreadingAPI::template future<SuccessElement>::type element_type;

		thread_observable()
		    : m_has_finished(false)
		{
		}

		explicit thread_observable(std::function<SuccessElement ()> action)
			: m_action(std::move(action))
			, m_has_finished(false)
		{
		}

		template <class Observer>
		void async_get_one(Observer &&observer)
		{
			if (m_has_finished)
			{
				return std::forward<Observer>(observer).ended();
			}
			assert(m_action);
			auto action = std::move(m_action);
			optional<typename std::decay<Observer>::type> maybe_observer{std::forward<Observer>(observer)};
			m_worker = ThreadingAPI::launch_async([
				this,
				SILICIUM_CAPTURE_EXPRESSION(maybe_observer, std::move(maybe_observer)),
				SILICIUM_CAPTURE_EXPRESSION(action, std::move(action))
				]() mutable
			{
				m_has_finished = true;
				typename ThreadingAPI::template packaged_task<SuccessElement>::type task(action);
				auto result = task.get_future();
				task();
				assert(maybe_observer);
				auto observer_ = std::forward<Observer>(*maybe_observer);
				std::forward<Observer>(observer_).got_element(std::move(result));
				//kill the observer with fire so that it cannot keep any shared state alive
				maybe_observer = none;
			});
		}

#if SILICIUM_COMPILER_GENERATES_MOVES
		thread_observable(thread_observable &&) = default;
		thread_observable &operator = (thread_observable &&) = default;
#else
		thread_observable(thread_observable &&other)
			: m_action(std::move(other.m_action))
			, m_worker(std::move(other.m_worker))
			, m_has_finished(std::move(other.m_has_finished))
		{
		}

		thread_observable &operator = (thread_observable &&other)
		{
			m_action = std::move(other.m_action);
			m_worker = std::move(other.m_worker);
			m_has_finished = std::move(other.m_has_finished);
			return *this;
		}
#endif

	private:

		typedef typename ThreadingAPI::template future<void>::type worker_type;

		std::function<SuccessElement ()> m_action;
		worker_type m_worker;
		bool m_has_finished;

		SILICIUM_DELETED_FUNCTION(thread_observable(thread_observable const &))
		SILICIUM_DELETED_FUNCTION(thread_observable &operator = (thread_observable const &))
	};

	template <class ThreadingAPI, class Action>
	auto make_thread_observable(Action &&action)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
		-> thread_observable<decltype(action()), ThreadingAPI>
#endif
	{
		return thread_observable<decltype(action()), ThreadingAPI>(std::forward<Action>(action));
	}
#endif
}

#endif
