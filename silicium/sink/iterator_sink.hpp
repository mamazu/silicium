#ifndef SILICIUM_ITERATOR_SINK_HPP
#define SILICIUM_ITERATOR_SINK_HPP

#include <silicium/sink/sink.hpp>
#include <silicium/config.hpp>
#include <silicium/success.hpp>

namespace Si
{
	template <class Element, class OutputIterator>
	struct iterator_sink
	{
		typedef Element element_type;
		typedef success error_type;

		iterator_sink()
		{
		}

		explicit iterator_sink(OutputIterator out)
			: m_out(std::move(out))
		{
		}

#if SILICIUM_COMPILER_GENERATES_MOVES
		iterator_sink(iterator_sink &&other) = default;
		iterator_sink &operator = (iterator_sink &&other) = default;
		iterator_sink(iterator_sink const &other) = default;
		iterator_sink &operator = (iterator_sink const &other) = default;
#else
		iterator_sink(iterator_sink &&other)
			: m_out(std::move(other.m_out))
		{
		}

		iterator_sink &operator = (iterator_sink &&other)
		{
			m_out = std::move(other.m_out);
			return *this;
		}

		iterator_sink(iterator_sink const &other)
			: m_out(other.m_out)
		{
		}

		iterator_sink &operator = (iterator_sink const &other)
		{
			m_out = other.m_out;
			return *this;
		}
#endif

		success append(iterator_range<Element const *> data) const
		{
			boost::range::copy(data, m_out);
			return {};
		}

	private:

		OutputIterator m_out;
	};

	template <class Element, class OutputIterator>
	auto make_iterator_sink(OutputIterator out)
		-> iterator_sink<Element, typename std::decay<OutputIterator>::type>
	{
		return iterator_sink<Element, typename std::decay<OutputIterator>::type>(std::move(out));
	}

	template <class Container>
	auto make_container_sink(Container &destination)
		-> iterator_sink<typename Container::value_type, std::back_insert_iterator<Container>>
	{
		return make_iterator_sink<typename Container::value_type>(std::back_inserter(destination));
	}
}

#endif
