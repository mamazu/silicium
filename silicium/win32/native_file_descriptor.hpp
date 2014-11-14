#ifndef SILICIUM_WIN32_NATIVE_FILE_DESCRIPTOR_HPP
#define SILICIUM_WIN32_NATIVE_FILE_DESCRIPTOR_HPP

#include <silicium/win32/win32.hpp>
#include <boost/config.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

namespace Si
{
	using native_file_descriptor = HANDLE;

	native_file_descriptor const no_file_handle = INVALID_HANDLE_VALUE;

	inline void terminating_close(native_file_descriptor file) BOOST_NOEXCEPT
	{
		if (!CloseHandle(file))
		{
			boost::throw_exception(boost::system::system_error(GetLastError(), boost::system::system_category()));
		}
	}
}

#endif