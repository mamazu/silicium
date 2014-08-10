#ifndef SILICIUM_REACTIVE_VARIANT_OBSERVABLE_HPP
#define SILICIUM_REACTIVE_VARIANT_OBSERVABLE_HPP

#include <silicium/observable.hpp>
#include <silicium/exchange.hpp>
#include <silicium/config.hpp>
#include <silicium/detail/integer_sequence.hpp>
#include <silicium/fast_variant.hpp>

namespace Si
{
#define SILICIUM_RX_VARIANT_AVAILABLE 1

#if SILICIUM_RX_VARIANT_AVAILABLE
	template <template <class ...T> class variant, class ...Parts>
	struct variant_observable : public Si::observable<variant<typename Parts::element_type...>>
	{
		typedef variant<typename Parts::element_type...> element_type;

		template <class ...P>
		explicit variant_observable(P &&...parts)
			: parts(std::forward<P>(parts)...)
		{
		}

		virtual void async_get_one(Si::observer<element_type> &receiver) SILICIUM_OVERRIDE
		{
			assert(!receiver_);
			receiver_ = &receiver;
			return async_get_one_impl<0, Parts...>();
		}

		virtual void cancel() SILICIUM_OVERRIDE
		{
			assert(receiver_);
			cancel_impl<0, sizeof...(Parts)>();
			receiver_ = nullptr;
		}

	private:

		template <class Element, std::size_t Index>
		struct tuple_observer : observer<Element>
		{
			variant_observable *combinator = nullptr;

			virtual void got_element(Element value) SILICIUM_OVERRIDE
			{
				cancel_others();
				Si::exchange(combinator->receiver_, nullptr)->got_element(element_type{std::move(value)});
			}

			virtual void ended() SILICIUM_OVERRIDE
			{
				cancel_others();
				Si::exchange(combinator->receiver_, nullptr)->ended();
			}

		private:

			void cancel_others()
			{
				combinator->cancel_impl<0, Index>();
				combinator->cancel_impl<Index + 1, sizeof...(Parts)>();
			}
		};

		template <class Indices>
		struct make_observers;

		template <std::size_t ...I>
		struct make_observers<ranges::v3::integer_sequence<I...>>
		{
			typedef std::tuple<tuple_observer<typename Parts::element_type, I>...> type;
		};

		typedef typename make_observers<typename ranges::v3::make_integer_sequence<sizeof...(Parts)>::type>::type observers_type;

		std::tuple<Parts...> parts;
		observers_type observers;
		Si::observer<element_type> *receiver_ = nullptr;

		template <std::size_t Index, class Head, class ...Tail>
		void async_get_one_impl()
		{
			auto &observer = std::get<Index>(observers);
			observer.combinator = this;
			auto &part = std::get<Index>(parts);
			part.async_get_one(observer);
			return async_get_one_impl<Index + 1, Tail...>();
		}

		template <std::size_t Index>
		void async_get_one_impl()
		{
		}

		template <std::size_t Begin, std::size_t End>
		void cancel_impl()
		{
			return cancel_impl<Begin, End>(std::integral_constant<bool, (Begin < End)>());
		}

		template <std::size_t Begin, std::size_t End>
		void cancel_impl(std::true_type)
		{
			assert(receiver_);
			auto &part = std::get<Begin>(parts);
			part.cancel();
			return cancel_impl<Begin + 1, End>();
		}

		template <std::size_t Begin, std::size_t End>
		void cancel_impl(std::false_type)
		{
		}
	};

	template <class ...Parts>
	auto make_variant(Parts &&...parts) -> variant_observable<Si::fast_variant, typename std::decay<Parts>::type...>
	{
		return variant_observable<Si::fast_variant, typename std::decay<Parts>::type...>(std::forward<Parts>(parts)...);
	}

	template <template <class ...T> class variant, class ...Parts>
	auto make_variant(Parts &&...parts) -> variant_observable<variant, typename std::decay<Parts>::type...>
	{
		return variant_observable<variant, typename std::decay<Parts>::type...>(std::forward<Parts>(parts)...);
	}
#endif
}

#endif
