#ifndef SILICIUM_MEMORY_RANGE_HPP
#define SILICIUM_MEMORY_RANGE_HPP

#include <silicium/iterator_range.hpp>
#include <boost/mpl/if.hpp>
#include <boost/type_traits/add_const.hpp>
#include <boost/type_traits/remove_const.hpp>
#include <boost/type_traits/is_const.hpp>
#include <boost/static_assert.hpp>
#include <boost/utility/addressof.hpp>

namespace Si
{
    typedef iterator_range<char const *> memory_range;
    typedef iterator_range<char *> mutable_memory_range;

    template <class To, class From>
    struct copy_const
    {
        typedef
            typename boost::mpl::if_<boost::is_const<From>,
                                     boost::add_const<To>,
                                     boost::remove_const<To>>::type::type type;
    };

    template <class Byte>
    auto make_memory_range(Byte *begin, Byte *end)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
        -> iterator_range<typename copy_const<char, Byte>::type *>
#endif
    {
        BOOST_STATIC_ASSERT(sizeof(Byte) == 1);
        BOOST_STATIC_ASSERT(std::is_pod<Byte>::value);
        typedef typename copy_const<char, Byte>::type destination;
        return make_iterator_range(reinterpret_cast<destination *>(begin),
                                   reinterpret_cast<destination *>(end));
    }

    template <class Byte>
    auto make_memory_range(Byte *data, std::size_t size)
        -> iterator_range<Byte *>
    {
        return make_memory_range(data, data + size);
    }

    template <class ContiguousRange>
    auto make_memory_range(ContiguousRange &&range)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
        -> decltype(make_memory_range(boost::addressof(*std::begin(range)),
                                      boost::addressof(*std::end(range))))
#endif
    {
        BOOST_STATIC_ASSERT(std::is_lvalue_reference<ContiguousRange>::value);
        using std::begin;
        using std::end;
        auto begin_ = begin(range);
        auto end_ = end(range);
        BOOST_STATIC_ASSERT(
            (std::is_same<std::random_access_iterator_tag,
                          typename std::iterator_traits<decltype(
                              begin_)>::iterator_category>::value));
        auto data = boost::addressof(*begin_);
        return make_memory_range(data, data + std::distance(begin_, end_));
    }

    template <class C>
    auto make_c_str_range(C const *str)
#if !SILICIUM_COMPILER_HAS_AUTO_RETURN_TYPE
        -> iterator_range<C const *>
#endif
    {
        return make_iterator_range(str, str + std::char_traits<C>::length(str));
    }
}

#endif
