#include <silicium/http/receive_request.hpp>
#include <silicium/http/generate_response.hpp>
#include <silicium/asio/writing_observable.hpp>
#include <silicium/asio/tcp_acceptor.hpp>
#include <silicium/observable/transform.hpp>
#include <silicium/observable/spawn_coroutine.hpp>
#include <silicium/observable/spawn_observable.hpp>
#include <silicium/sink/iterator_sink.hpp>
#include <silicium/terminate_on_exception.hpp>
#include <iostream>
#include <boost/algorithm/string/predicate.hpp>

#if SILICIUM_HAS_SPAWN_COROUTINE
namespace
{
	template <class YieldContext>
	void serve_client(boost::asio::ip::tcp::socket &client, YieldContext &&yield)
	{
		auto maybe_request = Si::http::receive_request(client, yield);
		if (maybe_request.is_error())
		{
			//The header was incomplete, maybe the connecting was closed.
			//If we want to know the reason, the error_extracting_source remembered it:
			boost::system::error_code error = maybe_request.error();
			boost::ignore_unused_variable_warning(error);
			return;
		}

		if (!maybe_request.get())
		{
			//syntax error in the request
			return;
		}

		Si::http::request const &request = *maybe_request.get();
		if (boost::algorithm::iequals(request.method, "CONNECT"))
		{
			boost::asio::ip::tcp::socket proxy_socket(client.get_io_service());
		}
		else
		{
			std::vector<char> response;
			{
				auto response_writer = Si::make_container_sink(response);
				Si::http::generate_status_line(response_writer, "HTTP/1.0", "200", "OK");
				boost::string_ref const content = "Hello, visitor!";
				Si::http::generate_header(response_writer, "Content-Length", boost::lexical_cast<Si::noexcept_string>(content.size()));
				Si::http::finish_headers(response_writer);
				Si::append(response_writer, content);
			}

			//you can handle the error if you want
			boost::system::error_code error = Si::asio::write(client, Si::make_memory_range(response), yield);
		}

		//ignore shutdown failures, they do not matter here
		boost::system::error_code error;
		client.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
	}

	boost::system::error_code spawn_server(boost::asio::io_service &io)
	{
		boost::system::error_code ec;

		//use a unique_ptr to support older versions of Boost where acceptor was not movable
		auto acceptor = Si::make_unique<boost::asio::ip::tcp::acceptor>(io);

		acceptor->open(boost::asio::ip::tcp::v4(), ec);
		if (ec)
		{
			return ec;
		}

		acceptor->bind(boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(), 8080), ec);
		if (ec)
		{
			return ec;
		}

		acceptor->listen(boost::asio::ip::tcp::acceptor::max_connections, ec);
		if (ec)
		{
			return ec;
		}

		Si::spawn_observable(
			Si::transform(
				Si::asio::make_tcp_acceptor(std::move(acceptor)),
				[](Si::asio::tcp_acceptor_result maybe_client)
				{
					auto client = maybe_client.get();
					Si::spawn_coroutine([client](Si::spawn_context yield)
					{
						serve_client(*client, yield);
					});
				}
			)
		);

		return {};
	}
}
#endif

int main()
{
	boost::asio::io_service io;
#if SILICIUM_HAS_SPAWN_COROUTINE
	boost::system::error_code ec = spawn_server(io);
	if (ec)
	{
		std::cerr << ec << ": " << ec.message() << '\n';
		return 1;
	}
#else
	std::cerr << "This example requires coroutine support\n";
#endif
	io.run();
}