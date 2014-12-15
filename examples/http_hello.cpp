#include <silicium/http/http.hpp>
#include <silicium/http/receive_request.hpp>
#include <silicium/asio/tcp_acceptor.hpp>
#include <silicium/asio/writing_observable.hpp>
#include <silicium/observable/transform.hpp>
#include <silicium/observable/flatten.hpp>
#include <silicium/observable/coroutine.hpp>
#include <silicium/observable/constant.hpp>
#include <silicium/observable/total_consumer.hpp>
#include <silicium/sink/iterator_sink.hpp>
#include <silicium/memory_range.hpp>

void serve_client(boost::asio::ip::tcp::socket &client, Si::yield_context yield)
{
	auto request = Si::http::receive_request(client, yield);
	if (request.is_error())
	{
		//The header was incomplete, maybe the connecting was closed.
		//If we want to know the reason, the error_extracting_source remembered it:
		boost::system::error_code error = request.error();
		boost::ignore_unused_variable_warning(error);
		return;
	}

	if (!request.get())
	{
		//syntax error in the request
		return;
	}

	std::vector<char> response;
	{
		auto response_writer = Si::make_container_sink(response);
		Si::http::generate_status_line(response_writer, "HTTP/1.0", "200", "OK");
		std::string const content = "Hello";
		Si::http::generate_header(response_writer, "Content-Length", boost::lexical_cast<Si::noexcept_string>(content.size()));
		Si::append(response_writer, "\r\n");
		Si::append(response_writer, content);
	}

	//you can handle the error if you want
	boost::system::error_code error = Si::asio::write(client, Si::make_memory_range(response), yield);

	//ignore shutdown failures, they do not matter here
	client.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
}

int main()
{
	boost::asio::io_service io;
	boost::asio::ip::tcp::acceptor acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(), 8080));
	auto accept_loop = Si::make_total_consumer(Si::flatten(Si::transform(Si::asio::make_tcp_acceptor(&acceptor), [](Si::asio::tcp_acceptor_result maybe_client)
	{
		auto client = maybe_client.get();
		auto client_handler = Si::make_coroutine([client](Si::yield_context yield) -> Si::nothing
		{
			serve_client(*client, yield);
			return {};
		});
		return Si::erase_unique(std::move(client_handler));
	})));
	accept_loop.start();
	io.run();
}
