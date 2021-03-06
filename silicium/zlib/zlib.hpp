#ifndef SILICIUM_ZLIB_ZLIB_HPP
#define SILICIUM_ZLIB_ZLIB_HPP

#include <silicium/config.hpp>
#include <boost/system/system_error.hpp>
#include <boost/throw_exception.hpp>

#if !SILICIUM_AVOID_ZLIB
#include <zlib.h>

namespace Si
{
    struct zlib_error_category : boost::system::error_category
    {
        zlib_error_category()
        {
        }

        virtual const char *name() const BOOST_NOEXCEPT SILICIUM_OVERRIDE
        {
            return "zlib";
        }

        virtual std::string message(int ev) const SILICIUM_OVERRIDE
        {
            switch (ev)
            {
#define SILICIUM_HANDLE_ERROR_CASE(error)                                      \
    case error:                                                                \
        return BOOST_STRINGIZE(error);
                SILICIUM_HANDLE_ERROR_CASE(Z_ERRNO)
                SILICIUM_HANDLE_ERROR_CASE(Z_STREAM_ERROR)
                SILICIUM_HANDLE_ERROR_CASE(Z_DATA_ERROR)
                SILICIUM_HANDLE_ERROR_CASE(Z_MEM_ERROR)
                SILICIUM_HANDLE_ERROR_CASE(Z_BUF_ERROR)
                SILICIUM_HANDLE_ERROR_CASE(Z_VERSION_ERROR)
#undef SILICIUM_HANDLE_ERROR_CASE
            default:
                return "";
            }
        }
    };

    inline boost::system::error_category const &zlib_category()
    {
        static zlib_error_category const instance;
        return instance;
    }

    inline void handle_zlib_status(int status)
    {
        if (status == Z_OK)
        {
            return;
        }
        boost::throw_exception(
            boost::system::system_error(status, zlib_category()));
    }
}
#endif

#endif
