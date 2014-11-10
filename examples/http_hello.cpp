#include <silicium/http/http.hpp>
#include <silicium/asio/tcp_acceptor.hpp>
#include <silicium/asio/socket_observable.hpp>
#include <silicium/asio/writing_observable.hpp>
#include <silicium/observable/transform.hpp>
#include <silicium/observable/flatten.hpp>
#include <silicium/observable/coroutine.hpp>
#include <silicium/observable/constant.hpp>
#include <silicium/source/virtualized_source.hpp>
#include <silicium/observable/total_consumer.hpp>
#include <silicium/memory_range.hpp>
#include <silicium/source/received_from_socket_source.hpp>
#include <silicium/source/observable_source.hpp>
#include <silicium/sink/iterator_sink.hpp>

void serve_client(boost::asio::ip::tcp::socket &client, Si::yield_context yield)
{
	std::array<char, 4096> incoming_buffer;
	auto receiver = Si::socket_observable(client, Si::make_memory_range(incoming_buffer));
	auto incoming_chunks = Si::virtualize_source(Si::make_observable_source(std::move(receiver), yield));
	Si::received_from_socket_source incoming_bytes(incoming_chunks);
	auto request_header = Si::http::parse_header(incoming_bytes);
	if (!request_header)
	{
		return;
	}
	std::cerr << request_header->method << " " << request_header->path << '\n';
	std::string content = "Hello";
	Si::http::response_header response_header;
	response_header.status = 200;
	response_header.status_text = "OK";
	response_header.http_version = "HTTP/1.0";
	response_header.arguments = Si::make_unique<Si::http::response_header::arguments_table>();
	(*response_header.arguments)["Content-Length"] = boost::lexical_cast<Si::noexcept_string>(content.size());
	std::vector<char> response;
	auto response_writer = Si::make_container_sink(response);
	Si::http::write_header(response_writer, response_header);
	Si::append(response_writer, content);
	auto sender = Si::make_writing_observable(client, Si::make_constant_observable(Si::make_memory_range(response)));
	yield.get_one(sender);
	client.shutdown(boost::asio::ip::tcp::socket::shutdown_both);
}

int main()
{
	boost::asio::io_service io;
	boost::asio::ip::tcp::acceptor acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(), 8080));
	auto accept_loop = Si::make_total_consumer(Si::flatten(Si::transform(Si::tcp_acceptor(acceptor), [](Si::tcp_acceptor_result maybe_client)
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
