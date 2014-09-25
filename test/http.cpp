#include <silicium/http/http.hpp>
#include <silicium/memory_source.hpp>
#include <boost/test/unit_test.hpp>

namespace Si
{
	BOOST_AUTO_TEST_CASE(http_parse_header)
	{
		std::string const incoming =
				"GET / HTTP/1.0\r\n"
				"Key: Value\r\n"
				"\r\n"
				;
		auto source = Si::make_container_source(incoming);
		boost::optional<Si::http::request_header> const parsed = Si::http::parse_header(source);
		BOOST_REQUIRE(parsed);
		BOOST_CHECK_EQUAL("GET", parsed->method);
		BOOST_CHECK_EQUAL("/", parsed->path);
		BOOST_CHECK_EQUAL("HTTP/1.0", parsed->http_version);
		std::map<noexcept_string, noexcept_string> const expected_arguments
		{
			{"Key", "Value"}
		};
		BOOST_CHECK(expected_arguments == parsed->arguments);
	}

	BOOST_AUTO_TEST_CASE(http_write_request_header)
	{
		std::string generated;
		auto sink = Si::make_container_sink(generated);
		http::request_header header;
		header.http_version = "HTTP/1.1";
		header.method = "POST";
		header.path = "/p";
		header.arguments["Content-Length"] = "13";
		http::write_header(sink, header);
		BOOST_CHECK_EQUAL(
			"POST /p HTTP/1.1\r\n"
			"Content-Length: 13\r\n"
			"\r\n",
			generated);
	}
}
