#ifndef SILICIUM_FUNCTION_OBSERVER_HPP
#define SILICIUM_FUNCTION_OBSERVER_HPP

#include <silicium/observable/observer.hpp>
#include <silicium/detail/argument_of.hpp>
#include <silicium/detail/element_from_optional_like.hpp>
#include <silicium/detail/proper_value_function.hpp>
#include <silicium/optional.hpp>
#include <boost/optional.hpp>

namespace Si
{
	namespace detail
	{
		template <class Element>
		struct optional_maker
		{
			typename std::decay<Element>::type *value;

			explicit optional_maker(typename std::decay<Element>::type *value)
			    : value(value)
			{
			}

			template <class T>
			operator boost::optional<T>() const
			{
				return static_cast<T>(std::forward<Element>(*value));
			}

			template <class T>
			operator Si::optional<T>() const
			{
				return static_cast<T>(std::forward<Element>(*value));
			}
		};
	}

	template <class Function>
	struct function_observer
	{
		typedef typename detail::element_from_optional_like<typename std::decay<
		    typename detail::argument_of<Function>::type>::type>::type
		    element_type;

		template <class F>
		explicit function_observer(
		    F &&function,
		    typename boost::enable_if_c<
		        !std::is_same<function_observer,
		                      typename std::decay<F>::type>::value,
		        void>::type * = nullptr)
		    : m_function(std::forward<F>(function))
		{
		}

#if SILICIUM_COMPILER_GENERATES_MOVES
		function_observer(function_observer &&) = default;
		function_observer(function_observer const &) = default;
		function_observer &operator=(function_observer &&) = default;
		function_observer &operator=(function_observer const &) = default;
#endif

		template <class Element>
		void got_element(Element &&element)
		{
			m_function(detail::optional_maker<Element>(&element));
		}

		void ended()
		{
			m_function(Si::none);
		}

	private:
#if SILICIUM_DETAIL_HAS_PROPER_VALUE_FUNCTION
		typedef typename detail::proper_value_function<
		    Function, void, typename detail::argument_of<Function>::type>::type
		    function_holder;
#else
		typedef Function function_holder;
#endif

		function_holder m_function;
	};

	BOOST_STATIC_ASSERT((
	    std::is_same<int,
	                 function_observer<void (*)(
	                     boost::optional<int> const &)>::element_type>::value));

	template <class Function>
	auto make_function_observer(Function &&function)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
	    -> function_observer<typename std::decay<Function>::type>
#endif
	{
		return function_observer<typename std::decay<Function>::type>(
		    std::forward<Function>(function));
	}
}

#endif
