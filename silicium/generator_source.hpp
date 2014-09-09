#ifndef SILICIUM_GENERATOR_SOURCE_HPP
#define SILICIUM_GENERATOR_SOURCE_HPP

#include <silicium/source.hpp>
#include <silicium/detail/proper_value_function.hpp>

namespace Si
{
	template <class Element, class Generator>
	struct generator_source
	{
		using element_type = Element;

		generator_source()
		{
		}

		explicit generator_source(Generator generate_next)
			: m_generate_next(std::move(generate_next))
		{
		}

		boost::iterator_range<Element const *> map_next(std::size_t size)
		{
			(void)size;
			return boost::iterator_range<Element const *>();
		}

		Element *copy_next(boost::iterator_range<Element *> destination)
		{
			auto copied = destination.begin();
			for (; copied != destination.end(); ++copied)
			{
				auto generated = m_generate_next();
				if (!generated)
				{
					break;
				}
				*copied = std::move(*generated);
			}
			return copied;
		}

		boost::uintmax_t minimum_size()
		{
			return 0;
		}

		boost::optional<boost::uintmax_t> maximum_size()
		{
			return boost::none;
		}

		std::size_t skip(std::size_t count)
		{
			std::size_t i = 0;
			for (; i < count; ++i)
			{
				if (!m_generate_next())
				{
					break;
				}
			}
			return i;
		}

	private:

		using proper_generator = typename detail::proper_value_function<Generator, boost::optional<Element>>::type;

		proper_generator m_generate_next;
	};

	template <class Element, class Generator>
	generator_source<Element, typename std::decay<Generator>::type> make_generator_source(Generator &&generate_next)
	{
		return generator_source<Element, typename std::decay<Generator>::type>{std::forward<Generator>(generate_next)};
	}
}

#endif