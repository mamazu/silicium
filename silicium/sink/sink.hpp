#ifndef SILICIUM_SINK_HPP
#define SILICIUM_SINK_HPP

#include <silicium/override.hpp>
#include <silicium/config.hpp>
#include <silicium/iterator_range.hpp>
#include <silicium/memory_range.hpp>
#include <boost/range/algorithm/copy.hpp>
#include <boost/filesystem/path.hpp>
#include <boost/utility/string_ref.hpp>
#include <boost/utility/explicit_operator_bool.hpp>
#include <boost/container/string.hpp>
#include <fstream>
#include <array>
#include <memory>

namespace Si
{
	template <class Element, class Error = boost::system::error_code>
	struct sink
	{
		typedef Element element_type;
		typedef Error error_type;

		virtual ~sink()
		{
		}

		virtual error_type append(iterator_range<element_type const *> data) = 0;
	};

	template <class Element, class Error = boost::system::error_code>
	struct null_sink : sink<Element, Error>
	{
		typedef Element element_type;
		typedef Error error_type;

		virtual error_type append(iterator_range<element_type const *> data) SILICIUM_OVERRIDE
		{
			boost::ignore_unused_variable_warning(data);
			return error_type();
		}
	};

	template <class Element, class Error = boost::system::error_code>
	struct buffer
	{
		typedef Element element_type;
		typedef Error error_type;

		virtual ~buffer()
		{
		}

		virtual iterator_range<element_type *> make_append_space(std::size_t size) = 0;
		virtual error_type flush_append_space() = 0;
	};

	struct ostream_ref_sink SILICIUM_FINAL : sink<char, void>
	{
		ostream_ref_sink()
			: m_file(nullptr)
		{
		}

		explicit ostream_ref_sink(std::ostream &file)
		   : m_file(&file)
		{
		}

		virtual void append(iterator_range<char const *> data) SILICIUM_OVERRIDE
		{
			assert(m_file);
			m_file->write(data.begin(), data.size());
		}

	private:

		std::ostream *m_file;
	};

	struct ostream_sink SILICIUM_FINAL : sink<char, boost::system::error_code>
	{
		//unique_ptr to make ostreams movable
		explicit ostream_sink(std::unique_ptr<std::ostream> file)
			: m_file(std::move(file))
		{
			m_file->exceptions(std::ios::failbit | std::ios::badbit);
		}

		virtual boost::system::error_code append(iterator_range<char const *> data) SILICIUM_OVERRIDE
		{
			m_file->write(data.begin(), data.size());
			return {};
		}

	private:

		std::unique_ptr<std::ostream> m_file;
	};

	inline std::unique_ptr<sink<char, boost::system::error_code>> make_file_sink(boost::filesystem::path const &name)
	{
		std::unique_ptr<std::ostream> file(new std::ofstream(name.string(), std::ios::binary));
		if (!*file)
		{
			throw std::runtime_error("Cannot open file for writing: " + name.string());
		}
		return std::unique_ptr<sink<char, boost::system::error_code>>(new ostream_sink(std::move(file)));
	}

	template <class Element, class Error>
	Error append(Si::sink<Element, Error> &out, std::basic_string<Element> const &str)
	{
		return out.append(make_iterator_range(str.data(), str.data() + str.size()));
	}

	template <class Element, class Error>
	Error append(Si::sink<Element, Error> &out, boost::container::basic_string<Element> const &str)
	{
		return out.append(make_iterator_range(str.data(), str.data() + str.size()));
	}

	template <class Element, class Error>
	Error append(Si::sink<Element, Error> &out, boost::basic_string_ref<Element> const &str)
	{
		return out.append(make_memory_range(str));
	}

	template <class Element, class Error>
	Error append(Si::sink<Element, Error> &out, Element const *c_str)
	{
		return out.append(make_iterator_range(c_str, c_str + std::char_traits<Element>::length(c_str)));
	}

	template <class Element, class Error>
	Error append(Si::sink<Element, Error> &out, Element const &single)
	{
		return out.append(make_iterator_range(&single, &single + 1));
	}

	struct success
	{
		bool operator !() const BOOST_NOEXCEPT
		{
			return true;
		}

		BOOST_EXPLICIT_OPERATOR_BOOL_NOEXCEPT()
	};
}

#endif
