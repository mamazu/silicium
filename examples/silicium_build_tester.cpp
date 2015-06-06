#include <silicium/async_process.hpp>
#include <silicium/http/generate_response.hpp>
#include <silicium/http/receive_request.hpp>
#include <silicium/asio/tcp_acceptor.hpp>
#include <silicium/asio/writing_observable.hpp>
#include <silicium/sink/iterator_sink.hpp>
#include <silicium/observable/spawn_coroutine.hpp>
#include <iostream>
#include <boost/program_options.hpp>

namespace
{
	void respond(boost::asio::ip::tcp::socket &client, Si::memory_range status, Si::memory_range status_text, Si::memory_range content, Si::spawn_context &yield)
	{
		std::vector<char> response;
		auto const response_sink = Si::make_container_sink(response);
		Si::http::generate_status_line(response_sink, "HTTP/1.1", status, status_text);
		Si::http::generate_header(response_sink, "Content-Length", boost::lexical_cast<Si::noexcept_string>(content.size()));
		Si::http::generate_header(response_sink, "Content-Type", "text/html");
		Si::http::finish_headers(response_sink);
		Si::append(response_sink, content);
		boost::system::error_code error = Si::asio::write(client, Si::make_memory_range(response), yield);
		if (!!error)
		{
			std::cerr << "Could not respond to " << client.remote_endpoint() << ": " << error << '\n';
		}
		else
		{
			client.shutdown(boost::asio::ip::tcp::socket::shutdown_both, error);
			if (!!error)
			{
				std::cerr << "Could not shutdown connection to " << client.remote_endpoint() << ": " << error << '\n';
			}
		}
	}

	void trigger_build()
	{
		std::cerr << "Build triggered (TODO)\n";
	}
}

int main(int argc, char **argv)
{
	std::string repository;
	std::string cache;
	boost::uint16_t listen_port = 8080;

	boost::program_options::options_description options("options");
	options.add_options()
		("help,h", "produce help message")
		("repository,r", boost::program_options::value(&repository), "the Git URI to clone from")
		("cache,c", boost::program_options::value(&cache), "the directory to use as a cache for the repository")
		("port,p", boost::program_options::value(&listen_port), "the port to listen on for HTTP requests")
		;
	boost::program_options::positional_options_description positional;
	positional.add("repository", 1);
	positional.add("cache", 1);
	boost::program_options::variables_map variables;
	try
	{
		boost::program_options::store(boost::program_options::command_line_parser(argc, argv).options(options).positional(positional).run(), variables);
		boost::program_options::notify(variables);
	}
	catch (boost::program_options::error const &ex)
	{
		std::cerr << options << '\n';
		std::cerr << ex.what() << '\n';
		return 1;
	}

	if (variables.count("help"))
	{
		std::cerr << options << '\n';
		return 0;
	}

	if (repository.empty() || cache.empty())
	{
		std::cerr << options << '\n';
		std::cerr << "Missing option\n";
		return 1;
	}

	boost::asio::io_service io;
	Si::spawn_coroutine([&io, listen_port](Si::spawn_context yield)
	{
		auto acceptor = Si::asio::make_tcp_acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(), listen_port));
		for (;;)
		{
			std::shared_ptr<boost::asio::ip::tcp::socket> client = (*yield.get_one(Si::ref(acceptor))).get();
			assert(client);
			Si::spawn_coroutine([client = std::move(client)](Si::spawn_context yield)
			{
				Si::error_or<Si::optional<Si::http::request>> const received = Si::http::receive_request(*client, yield);
				if (received.is_error())
				{
					std::cerr << "Error when receiving request from " << client->remote_endpoint() << ": " << received.error() << '\n';
					return;
				}

				Si::optional<Si::http::request> const &maybe_request = received.get();
				if (!maybe_request)
				{
					return respond(*client, Si::make_c_str_range("400"), Si::make_c_str_range("Bad Request"), Si::make_c_str_range("the server could not parse the request"), yield);
				}

				Si::http::request const &request = *maybe_request;
				if (request.method != "POST" && request.method != "GET")
				{
					return respond(*client, Si::make_c_str_range("405"), Si::make_c_str_range("Method Not Allowed"), Si::make_c_str_range("this HTTP method is not supported by this server"), yield);
				}

				if (request.path != "/")
				{
					return respond(*client, Si::make_c_str_range("404"), Si::make_c_str_range("Not Found"), Si::make_c_str_range("unknown path"), yield);
				}

				if (request.method == "POST")
				{
					trigger_build();
				}

				char const * const page =
					"<html>"
						"<body>"
							"<form action=\"/\" method=\"POST\">"
								"<input type=\"submit\" value=\"Trigger build\"/>"
							"</form>"
						"</body>"
					"</html>";
				return respond(*client, Si::make_c_str_range("200"), Si::make_c_str_range("OK"), Si::make_c_str_range(page), yield);
			});
		}
	});
	io.run();
}
