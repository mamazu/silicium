#ifndef SILICIUM_SUCCESS_HPP
#define SILICIUM_SUCCESS_HPP

#include <silicium/explicit_operator_bool.hpp>

namespace Si
{
	struct success
	{
		bool operator !() const BOOST_NOEXCEPT
		{
			return true;
		}

		SILICIUM_EXPLICIT_OPERATOR_BOOL()
	};
}

#endif
