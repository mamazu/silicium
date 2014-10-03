#ifndef SILICIUM_ERROR_OR_HPP
#define SILICIUM_ERROR_OR_HPP

#include <silicium/optional.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>
#include <system_error>

namespace Si
{
	namespace detail
	{
		inline SILICIUM_NORETURN void throw_system_error(boost::system::error_code error)
		{
			boost::throw_exception(boost::system::system_error(error));
		}

		inline SILICIUM_NORETURN void throw_system_error(std::error_code error)
		{
			boost::throw_exception(std::system_error(error));
		}
	}

	template <class Value, class Error = boost::system::error_code>
	struct error_or
	{
		error_or() BOOST_NOEXCEPT
		{
		}

		template <class ConvertibleToValue, class = typename std::enable_if<std::is_convertible<ConvertibleToValue, Value>::value, void>::type>
		error_or(ConvertibleToValue &&value) BOOST_NOEXCEPT
			: storage(Value(std::forward<ConvertibleToValue>(value)))
		{
		}

		error_or(Error error) BOOST_NOEXCEPT
			: storage(std::move(error))
		{
		}

#ifdef _MSC_VER
		error_or(error_or &&other) BOOST_NOEXCEPT
			: storage(std::move(other.storage))
		{
		}

		error_or(error_or const &other)
			: storage(other.storage)
		{
		}

		error_or &operator = (error_or &&other) BOOST_NOEXCEPT
		{
			storage = std::move(other.storage);
			return *this;
		}

		error_or &operator = (error_or const &other)
		{
			storage = other.storage;
			return *this;
		}
#endif

		bool is_error() const BOOST_NOEXCEPT
		{
			return Si::visit<bool>(
						storage,
						[](Value const &) { return false; },
						[](Error const &) { return true; });
		}

		Si::optional<Error> error() const BOOST_NOEXCEPT
		{
			return Si::visit<Si::optional<Error>>(
						storage,
						[](Value const &) { return Si::optional<Error>(); },
						[](Error const &e) { return e; });
		}

#if SILICIUM_COMPILER_HAS_RVALUE_THIS_QUALIFIER
		Value &get() &
		{
			return Si::visit<Value &>(
						storage,
						[](Value &value) -> Value & { return value; },
						[](Error const &e) -> Value & { detail::throw_system_error(e); });
		}
#endif

		Value &&get()
#if SILICIUM_COMPILER_HAS_RVALUE_THIS_QUALIFIER
			&&
#endif
		{
			return Si::visit<Value &&>(
						storage,
						[](Value &value) -> Value && { return std::move(value); },
						[](Error const &e) -> Value && { detail::throw_system_error(e); });
		}

		Value const &get() const
#if SILICIUM_COMPILER_HAS_RVALUE_THIS_QUALIFIER
			&
#endif
		{
			return Si::visit<Value const &>(
						storage,
						[](Value const &value) -> Value const & { return value; },
						[](Error const &e) -> Value const & { detail::throw_system_error(e); });
		}

		Value *get_ptr() BOOST_NOEXCEPT
		{
			return Si::visit<Value *>(
						storage,
						[](Value &value) -> Value * { return &value; },
						[](Error const &) -> Value * { return nullptr; });
		}

		Value const *get_ptr() const BOOST_NOEXCEPT
		{
			return Si::visit<Value const *>(
						storage,
						[](Value const &value) -> Value const * { return &value; },
						[](Error const &) -> Value const * { return nullptr; });
		}

		Value *operator -> ()
		{
			return &get();
		}

		Value const *operator -> () const
		{
			return &get();
		}

		boost::optional<Value> get_optional()
#if SILICIUM_COMPILER_HAS_RVALUE_THIS_QUALIFIER
			&&
#endif
		{
			auto *value = get_ptr();
			if (!value)
			{
				return boost::none;
			}
			return std::move(*value);
		}

		bool equals(error_or const &other) const
		{
			return storage == other.storage;
		}

	private:

		fast_variant<Value, Error> storage;
	};

	namespace detail
	{
		template <class To, class From>
		From &&convert_if_necessary(From &&from, std::true_type)
		{
			return std::forward<From>(from);
		}

		template <class To, class From>
		To convert_if_necessary(From &&from, std::false_type)
		{
			return To(std::forward<From>(from));
		}
	}

	template <class Value, class Error>
	bool operator == (error_or<Value, Error> const &left, error_or<Value, Error> const &right)
	{
		return left.equals(right);
	}

	template <class Value, class Error, class ConvertibleToValue, class = typename std::enable_if<std::is_convertible<ConvertibleToValue, Value>::value, void>::type>
	bool operator == (error_or<Value, Error> const &left, ConvertibleToValue const &right)
	{
		if (left.is_error())
		{
			return false;
		}
		return left.get() == detail::convert_if_necessary<Value>(right, std::is_same<typename std::decay<Value>::type, typename std::decay<ConvertibleToValue>::type>());
	}

	template <class Value, class Error>
	bool operator == (error_or<Value, Error> const &left, Error const &right)
	{
		if (!left.is_error())
		{
			return false;
		}
		return *left.error() == right;
	}

	template <class Anything, class Value, class Error>
	bool operator == (Anything const &left, error_or<Value, Error> const &right)
	{
		return (right == left);
	}

	template <class Anything, class Value, class Error>
	bool operator != (Anything const &left, error_or<Value, Error> const &right)
	{
		return !(left == right);
	}

	template <class Anything, class Value, class Error>
	bool operator != (error_or<Value, Error> const &left, Anything const &right)
	{
		return !(left == right);
	}

	template <class Value, class Error>
	std::ostream &operator << (std::ostream &out, error_or<Value, Error> const &value)
	{
		if (value.error())
		{
			return out << *value.error();
		}
		return out << value.get();
	}

	template <class T>
	struct is_error_or : std::false_type
	{
	};

	template <class Value, class Error>
	struct is_error_or<error_or<Value, Error>> : std::true_type
	{
	};

	template <class ErrorOr, class OnValue, class CleanErrorOr = typename std::decay<ErrorOr>::type, class = typename std::enable_if<is_error_or<CleanErrorOr>::value, void>>
	auto map(ErrorOr &&maybe, OnValue &&on_value) -> CleanErrorOr
	{
		if (maybe.is_error())
		{
			return *maybe.error();
		}
		return std::forward<OnValue>(on_value)(std::forward<ErrorOr>(maybe).get());
	}
}

#endif
