#ifndef SILICIUM_THROWING_SINK_HPP
#define SILICIUM_THROWING_SINK_HPP

#include <silicium/sink/sink.hpp>
#include <silicium/config.hpp>
#include <boost/throw_exception.hpp>
#include <boost/system/system_error.hpp>

namespace Si
{
    template <class Next>
    struct throwing_sink
    {
        typedef typename Next::element_type element_type;
        typedef success error_type;

        throwing_sink()
        {
        }

        explicit throwing_sink(Next next)
            : next(next)
        {
        }

        error_type append(iterator_range<element_type const *> data)
        {
            auto error = next.append(data);
            if (error)
            {
                boost::throw_exception(boost::system::system_error(error));
            }
            return {};
        }

    private:
        Next next;
    };

    template <class Next>
    auto make_throwing_sink(Next &&next)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
        -> throwing_sink<typename std::decay<Next>::type>
#endif
    {
        return throwing_sink<typename std::decay<Next>::type>(
            std::forward<Next>(next));
    }
}

#endif
