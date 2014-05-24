#ifndef SILICIUM_PROCESS_HPP
#define SILICIUM_PROCESS_HPP

#include <vector>
#include <string>
#include <boost/optional.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/range/iterator_range.hpp>
#include <boost/range/algorithm/copy.hpp>

namespace Si
{
	struct process_output
	{
		int exit_status;
		//TODO: make stdout asynchronously readable
		boost::optional<std::vector<char>> stdout;
	};

	process_output run_process(std::string executable, std::vector<std::string> arguments, boost::filesystem::path const &current_path, bool dump_stdout);

	template <class Element>
	struct sink
	{
		virtual ~sink()
		{
		}

		virtual boost::iterator_range<Element *> make_append_space(std::size_t size) = 0;
		virtual void flush_append_space() = 0;
		virtual void append(boost::iterator_range<Element const *> data) = 0;
	};

	template <class Element>
	void commit(sink<Element> &destination, std::size_t count)
	{
		destination.make_append_space(count);
		destination.flush_append_space();
	}

	template <class Element, std::size_t BufferSize>
	struct appender
	{
		explicit appender(sink<Element> &destination)
			: m_destination(destination)
		{
		}

		boost::iterator_range<Element *> make_append_space(std::size_t size)
		{
			auto first_try = m_destination.make_append_space(size);
			if (!first_try.empty())
			{
				return first_try;
			}
			m_using_fallback = true;
			return boost::make_iterator_range(m_fallback_buffer);
		}

		void commit(std::size_t count)
		{
			if (m_using_fallback)
			{
				assert(count < m_fallback_buffer.size());
				m_destination.append(boost::make_iterator_range(m_fallback_buffer.data(), m_fallback_buffer.data() + count));
			}
			else
			{
				Si::commit(m_destination, count);
			}
		}

	private:

		sink<Element> &m_destination;
		std::array<Element, BufferSize> m_fallback_buffer;
		bool m_using_fallback = false;
	};

	template <class Element, class OutputIterator>
	struct iterator_sink : sink<Element>
	{
		explicit iterator_sink(OutputIterator out)
			: m_out(std::move(out))
		{
		}

		virtual boost::iterator_range<Element *> make_append_space(std::size_t) override
		{
			return {};
		}

		virtual void flush_append_space() override
		{
		}

		virtual void append(boost::iterator_range<Element const *> data) override
		{
			boost::range::copy(data, m_out);
		}

	private:

		OutputIterator m_out;
	};

	template <class Element, class OutputIterator>
	auto make_iterator_sink(OutputIterator out)
	{
		return iterator_sink<Element, typename std::decay<OutputIterator>::type>(std::move(out));
	}

	template <class Element>
	struct source
	{
		virtual ~source()
		{
		}

		virtual boost::iterator_range<Element const *> map_next(std::size_t size) = 0;
		virtual Element *copy_next(boost::iterator_range<Element *> destination) = 0;
	};

	struct process_parameters
	{
		std::string executable;
		std::vector<std::string> arguments;
		boost::filesystem::path current_path;
		std::unique_ptr<sink<char>> stdout;
		std::unique_ptr<sink<char>> stderr;
		std::unique_ptr<source<char>> stdin;
	};

	int run_process(process_parameters const &parameters);
}

#endif
