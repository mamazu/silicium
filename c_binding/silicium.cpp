#include "silicium.h"
#include <silicium/observable/virtualized.hpp>
#include <silicium/observable/coroutine.hpp>

struct silicium_receiver : Si::observer<void *>
{
	silicium_observer_function callback;
	void *user_data;

	silicium_receiver()
		: callback(nullptr)
		, user_data(nullptr)
	{
	}

	silicium_receiver(silicium_observer_function callback, void *user_data)
		: callback(callback)
		, user_data(user_data)
	{
	}

	virtual void got_element(void *value) SILICIUM_OVERRIDE
	{
		assert(callback);
		callback(user_data, value);
	}

	virtual void ended() SILICIUM_OVERRIDE
	{
		assert(callback);
		callback(user_data, nullptr);
	}
};

struct silicium_observable : Si::observable<void *>
{
	silicium_receiver receiver;
};

struct silicium_coroutine : silicium_observable
{
	template <class ...Args>
	explicit silicium_coroutine(Args &&...args)
		: m_coroutine(std::forward<Args>(args)...)
	{
	}

	virtual void async_get_one(Si::observer<void *> &receiver) SILICIUM_OVERRIDE
	{
		m_coroutine.async_get_one(receiver);
	}

private:

	Si::coroutine_observable<void *> m_coroutine;
};

struct silicium_yield_context
{

};

extern "C"
silicium_observable *silicium_make_coroutine(silicium_coroutine_function action, void *user_data)
{
	assert(action);
	try
	{
		auto result = Si::make_unique<silicium_coroutine>([action, user_data](Si::yield_context yield) -> void *
		{
			silicium_yield_context wrapped_yield;
			return action(user_data, &wrapped_yield);
		});
		return result.release();
	}
	catch (std::bad_alloc const &)
	{
		return nullptr;
	}
}

extern "C"
void silicium_async_get_one(silicium_observable *observable, silicium_observer_function callback, void *user_data)
{
	assert(observable);
	assert(callback);
	observable->receiver = silicium_receiver(callback, user_data);
	return observable->async_get_one(observable->receiver);
}

extern "C"
void silicium_free_observable(silicium_observable *observable)
{
	std::unique_ptr<silicium_observable>{observable};
}