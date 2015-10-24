#ifndef SILICIUM_BOUNDED_INT_HPP
#define SILICIUM_BOUNDED_INT_HPP

#include <silicium/optional.hpp>

namespace Si
{
	template <class Int, Int Minimum, Int Maximum>
	struct bounded_int
	{
		typedef Int value_type;

		BOOST_STATIC_ASSERT(Minimum <= Maximum);

		template <Int OtherMinimum, Int OtherMaximum>
		bounded_int(bounded_int<Int, OtherMinimum, OtherMaximum> const &other)
		    : m_value(other.value())
		{
			BOOST_STATIC_ASSERT(OtherMinimum >= Minimum);
			BOOST_STATIC_ASSERT(OtherMaximum <= Maximum);
		}

		static optional<bounded_int> create(Int possible_value)
		{
			int minimum = Minimum;
			if (possible_value < minimum)
			{
				return none;
			}
			int maximum = Maximum;
			if (possible_value > maximum)
			{
				return none;
			}
			return bounded_int(possible_value);
		}

		template <Int Literal>
		static bounded_int literal()
		{
			BOOST_STATIC_ASSERT(Literal >= Minimum);
			BOOST_STATIC_ASSERT(Literal <= Maximum);
			return bounded_int(Literal);
		}

		Int value() const
		{
			return m_value;
		}

		template <Int NewMinimum, Int NewMaximum>
		bounded_int<Int, NewMinimum, NewMaximum> clamp() const
		{
			using std::min;
			using std::max;
			auto result = bounded_int<Int, NewMinimum, NewMaximum>::create(max(NewMinimum, min(NewMaximum, m_value)));
			assert(result);
			return *result;
		}

		template <Int NewMinimum, Int NewMaximum>
		optional<bounded_int<Int, NewMinimum, NewMaximum>> narrow() const
		{
			return bounded_int<Int, NewMinimum, NewMaximum>::create(m_value);
		}

	private:
		Int m_value;

		explicit bounded_int(Int value)
		    : m_value(value)
		{
		}
	};

	template <class Int, Int MinimumLeft, Int MaximumLeft, Int MinimumRight, Int MaximumRight>
	bool operator==(bounded_int<Int, MinimumLeft, MaximumLeft> const &left,
	                bounded_int<Int, MinimumRight, MaximumRight> const &right)
	{
		BOOST_STATIC_ASSERT(((MaximumLeft > MinimumRight) && (MinimumLeft <= MinimumRight)) ||
		                    ((MaximumRight > MinimumLeft) && (MinimumRight <= MinimumLeft)));
		return left.value() == right.value();
	}

	template <class Int, Int MinimumLeft, Int MaximumLeft, Int MinimumRight, Int MaximumRight>
	bool operator<(bounded_int<Int, MinimumLeft, MaximumLeft> const &left,
	               bounded_int<Int, MinimumRight, MaximumRight> const &right)
	{
		BOOST_STATIC_ASSERT(((MaximumLeft > MinimumRight) && (MinimumLeft <= MinimumRight)) ||
		                    ((MaximumRight > MinimumLeft) && (MinimumRight <= MinimumLeft)));
		return left.value() < right.value();
	}

	template <class Int, Int Minimum, Int Maximum>
	std::ostream &operator<<(std::ostream &out, bounded_int<Int, Minimum, Maximum> const &value)
	{
		// Propagate char types to int by adding zero so that ostream will properly format them as numbers.
		auto printable = (0 + value.value());
		return out << printable;
	}

	template <class Int, Int MinimumLeft, Int MaximumLeft, Int MinimumRight, Int MaximumRight>
	bounded_int<Int, MinimumLeft + MinimumRight, MaximumLeft + MaximumRight>
	operator+(bounded_int<Int, MinimumLeft, MaximumLeft> const &left,
	          bounded_int<Int, MinimumRight, MaximumRight> const &right)
	{
		auto result = bounded_int<Int, MinimumLeft + MinimumRight, MaximumLeft + MaximumRight>::create(left.value() +
		                                                                                               right.value());
		assert(result);
		return *result;
	}
}

#endif
