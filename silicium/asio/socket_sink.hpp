#ifndef SILICIUM_ASIO_SOCKET_SINK_HPP
#define SILICIUM_ASIO_SOCKET_SINK_HPP

#include <silicium/sink/sink.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/write.hpp>

#if BOOST_VERSION >= 105400
#include <boost/asio/spawn.hpp>

namespace Si
{
	namespace asio
	{
		struct socket_sink : Si::sink<char, boost::system::error_code>
		{
			explicit socket_sink(boost::asio::ip::tcp::socket &socket, boost::asio::yield_context &yield);
			virtual boost::system::error_code append(iterator_range<char const *> data) SILICIUM_OVERRIDE;

		private:

			boost::asio::ip::tcp::socket *m_socket;
			boost::asio::yield_context *m_yield;
		};

		inline socket_sink::socket_sink(boost::asio::ip::tcp::socket &socket, boost::asio::yield_context &yield)
			: m_socket(&socket)
			, m_yield(&yield)
		{
		}

		inline boost::system::error_code socket_sink::append(iterator_range<char const *> data)
		{
			assert(m_socket);
			assert(m_yield);
			boost::system::error_code ec;
			boost::asio::async_write(*m_socket, boost::asio::buffer(data.begin(), data.size()), (*m_yield)[ec]);
			return ec;
		}
	}
}
#endif

#endif
