#include <reactive/coroutine.hpp>
#include <silicium/http/http.hpp>
#include <silicium/fast_variant.hpp>
#include <reactive/finite_state_machine.hpp>
#include <reactive/generate.hpp>
#include <reactive/total_consumer.hpp>
#include <boost/asio.hpp>
#include <boost/ref.hpp>
#include <boost/range/algorithm/find_if.hpp>
#include <boost/format.hpp>

namespace rx
{
	using tcp_acceptor_result = Si::fast_variant<
		std::shared_ptr<boost::asio::ip::tcp::socket>, //until socket itself is noexcept-movable
		boost::system::error_code
	>;

	struct tcp_acceptor : observable<tcp_acceptor_result>, boost::noncopyable
	{
		typedef tcp_acceptor_result element_type;

		explicit tcp_acceptor(boost::asio::ip::tcp::acceptor &underlying)
			: underlying(underlying)
		{
		}

		~tcp_acceptor()
		{
			if (!receiver_)
			{
				return;
			}
			underlying.cancel();
		}

		virtual void async_get_one(observer<element_type> &receiver) SILICIUM_OVERRIDE
		{
			assert(!receiver_);
			next_client = std::make_shared<boost::asio::ip::tcp::socket>(underlying.get_io_service());
			underlying.async_accept(*next_client, [this](boost::system::error_code error)
			{
				if (!this->receiver_)
				{
					//can happen when cancel has been called on the observable when the callback was
					//already posted to the io_service
					return;
				}
				if (error)
				{
					if (error == boost::asio::error::operation_aborted)
					{
						return;
					}
					exchange(this->receiver_, nullptr)->got_element(tcp_acceptor_result{error});
				}
				else
				{
					exchange(this->receiver_, nullptr)->got_element(tcp_acceptor_result{std::move(next_client)});
				}
			});
			receiver_ = &receiver;
		}

		virtual void cancel() SILICIUM_OVERRIDE
		{
			assert(receiver_);
			underlying.cancel();
			receiver_ = nullptr;
		}

	private:

		boost::asio::ip::tcp::acceptor &underlying;
		std::shared_ptr<boost::asio::ip::tcp::socket> next_client;
		observer<element_type> *receiver_ = nullptr;
	};

	template <class Observable, class YieldContext>
	struct observable_source : Si::source<typename Observable::element_type>
	{
		typedef typename Observable::element_type element_type;

		observable_source(Observable input, YieldContext &yield)
			: input(std::move(input))
			, yield(yield)
		{
		}

		virtual boost::iterator_range<element_type const *> map_next(std::size_t size) SILICIUM_OVERRIDE
		{
			(void)size;
			return {};
		}

		virtual element_type *copy_next(boost::iterator_range<element_type *> destination) SILICIUM_OVERRIDE
		{
			using boost::begin;
			using boost::end;
			auto i = begin(destination);
			for (; i != end(destination); ++i)
			{
				auto element = yield.get_one(input);
				if (!element)
				{
					break;
				}
				*i = std::move(*element);
			}
			return i;
		}

		virtual boost::uintmax_t minimum_size() SILICIUM_OVERRIDE
		{
			return 0;
		}

		virtual boost::optional<boost::uintmax_t> maximum_size() SILICIUM_OVERRIDE
		{
			return boost::none;
		}

		virtual std::size_t skip(std::size_t count) SILICIUM_OVERRIDE
		{
			size_t skipped = 0;
			while ((skipped < count) && yield.get_one(input))
			{
			}
			return skipped;
		}

	private:

		Observable input;
		YieldContext &yield;
	};

	template <class Observable, class YieldContext>
	auto make_observable_source(Observable &&input, YieldContext &yield) -> observable_source<typename std::decay<Observable>::type, YieldContext>
	{
		return observable_source<typename std::decay<Observable>::type, YieldContext>(std::forward<Observable>(input), yield);
	}

	typedef Si::fast_variant<std::size_t, boost::system::error_code> received_from_socket;

	struct socket_observable : observable<received_from_socket>
	{
		typedef received_from_socket element_type;
		typedef boost::iterator_range<char *> buffer_type;

		socket_observable(boost::asio::ip::tcp::socket &socket, buffer_type buffer)
			: socket(&socket)
			, buffer(buffer)
		{
			assert(!buffer.empty());
		}

		virtual void async_get_one(observer<element_type> &receiver) SILICIUM_OVERRIDE
		{
			assert(!receiver_);
			socket->async_receive(boost::asio::buffer(buffer.begin(), buffer.size()), [this](boost::system::error_code error, std::size_t bytes_received)
			{
				if (error)
				{
					if (error == boost::asio::error::operation_aborted)
					{
						return;
					}
					exchange(this->receiver_, nullptr)->got_element(error);
				}
				else
				{
					exchange(this->receiver_, nullptr)->got_element(bytes_received);
				}
			});
			receiver_ = &receiver;
		}

		virtual void cancel() SILICIUM_OVERRIDE
		{
			assert(receiver_);
			socket->cancel();
			receiver_ = nullptr;
		}

	private:

		boost::asio::ip::tcp::socket *socket;
		buffer_type buffer;
		observer<element_type> *receiver_ = nullptr;
	};

	struct socket_receiver_observable : observable<char>, private observer<received_from_socket>
	{
		typedef char element_type;
		typedef boost::iterator_range<char *> buffer_type;

		explicit socket_receiver_observable(boost::asio::ip::tcp::socket &socket, buffer_type buffer)
			: socket(&socket)
			, buffer(buffer)
			, next_byte(buffer.begin())
			, available(buffer.begin())
		{
		}

		virtual void async_get_one(observer<element_type> &receiver) SILICIUM_OVERRIDE
		{
			assert(!receiver_);
			if (next_byte != buffer.end())
			{
				char value = *next_byte;
				++next_byte;
				return receiver.got_element(value);
			}
			receiving = boost::in_place(boost::ref(*socket), buffer);
			receiving->async_get_one(*this);
			receiver_ = &receiver;
		}

		virtual void cancel() SILICIUM_OVERRIDE
		{
			throw std::logic_error("to do");
		}

	private:

		boost::asio::ip::tcp::socket *socket;
		buffer_type buffer;
		buffer_type::iterator next_byte;
		buffer_type::iterator available;
		observer<element_type> *receiver_ = nullptr;
		boost::optional<socket_observable> receiving;

		struct received_from_handler : boost::static_visitor<>
		{
			socket_receiver_observable *receiver;

			explicit received_from_handler(socket_receiver_observable &receiver)
				: receiver(&receiver)
			{
			}

			void operator()(boost::system::error_code) const
			{
				//error means end
				return rx::exchange(receiver->receiver_, nullptr)->ended();
			}

			void operator()(std::size_t bytes_received) const
			{
				assert(bytes_received > 0);
				assert(static_cast<ptrdiff_t>(bytes_received) <= receiver->buffer.size());
				receiver->available = receiver->buffer.begin() + bytes_received;
				receiver->next_byte = receiver->buffer.begin();
				char value = *receiver->next_byte;
				++(receiver->next_byte);
				return rx::exchange(receiver->receiver_, nullptr)->got_element(value);
			}
		};

		virtual void got_element(received_from_socket value) SILICIUM_OVERRIDE
		{
			received_from_handler handler{*this};
			return Si::apply_visitor(handler, value);
		}

		virtual void ended() SILICIUM_OVERRIDE
		{
			throw std::logic_error("should not be called");
		}
	};

	template <class T, class ...Args>
	auto make_unique(Args &&...args)
	{
		return std::unique_ptr<T>(new T(std::forward<Args>(args)...));
	}

	using nothing = detail::nothing;

	template <class NothingObservableObservable>
	struct flattener
			: public observable<nothing>
			, private observer<typename NothingObservableObservable::element_type>
	{
		explicit flattener(NothingObservableObservable input)
			: input(std::move(input))
		{
		}

		virtual void async_get_one(observer<element_type> &receiver) SILICIUM_OVERRIDE
		{
			assert(!receiver_);
			receiver_ = &receiver;
			fetch();
		}

		virtual void cancel() SILICIUM_OVERRIDE
		{
			throw std::logic_error("to do");
		}

	private:

		typedef typename NothingObservableObservable::element_type nothing_observable;

		struct child : observer<nothing>
		{
			flattener &parent;
			nothing_observable observed;

			explicit child(flattener &parent, nothing_observable observed)
				: parent(parent)
				, observed(observed)
			{
			}

			void start()
			{
				observed.async_get_one(*this);
			}

			virtual void got_element(nothing) SILICIUM_OVERRIDE
			{
				return start();
			}

			virtual void ended() SILICIUM_OVERRIDE
			{
				return parent.remove_child(*this);
			}
		};

		NothingObservableObservable input;
		bool input_ended = false;
		observer<nothing> *receiver_ = nullptr;
		std::vector<std::unique_ptr<child>> children;

		void fetch()
		{
			return input.async_get_one(*this);
		}

		void remove_child(child &removing)
		{
			auto const i = boost::range::find_if(children, [&removing](std::unique_ptr<child> const &element)
			{
				return element.get() == &removing;
			});
			children.erase(i);
			if (input_ended &&
			    children.empty())
			{
				exchange(receiver_, nullptr)->ended();
			}
		}

		virtual void got_element(nothing_observable value) SILICIUM_OVERRIDE
		{
			children.emplace_back(make_unique<child>(*this, std::move(value)));
			child &new_child = *children.back();
			new_child.start();
			return fetch();
		}

		virtual void ended() SILICIUM_OVERRIDE
		{
			assert(receiver_);
			input_ended = true;
			if (children.empty())
			{
				exchange(receiver_, nullptr)->ended();
			}
		}
	};

	template <class NothingObservableObservable>
	auto flatten(NothingObservableObservable &&input)
	{
		return flattener<typename std::decay<NothingObservableObservable>::type>(std::forward<NothingObservableObservable>(input));
	}

	struct sending_observable : observable<boost::system::error_code>
	{
		typedef boost::system::error_code element_type;
		typedef boost::iterator_range<char const *> buffer_type;

		explicit sending_observable(boost::asio::ip::tcp::socket &socket, buffer_type buffer)
			: socket(&socket)
			, buffer(buffer)
		{
		}

		virtual void async_get_one(observer<boost::system::error_code> &receiver) SILICIUM_OVERRIDE
		{
			assert(!receiver_);
			if (buffer.empty())
			{
				return receiver.ended();
			}
			receiver_ = &receiver;
			boost::asio::async_write(*socket, boost::asio::buffer(buffer.begin(), buffer.size()), [this](boost::system::error_code error, std::size_t bytes_sent)
			{
				assert(buffer.size() == static_cast<ptrdiff_t>(bytes_sent));
				buffer = buffer_type();
				return exchange(receiver_, nullptr)->got_element(error);
			});
		}

		virtual void cancel() SILICIUM_OVERRIDE
		{
			assert(receiver_);
			throw std::logic_error("to do");
		}

	private:

		boost::asio::ip::tcp::socket *socket = nullptr;
		buffer_type buffer;
		observer<boost::system::error_code> *receiver_ = nullptr;
	};
}

namespace
{
	using events = rx::shared_observable<rx::detail::nothing>;

	void serve_client(boost::asio::ip::tcp::socket &client, rx::yield_context<rx::detail::nothing> &yield, boost::uintmax_t visitor_number)
	{
		std::vector<char> received(4096);
		rx::socket_receiver_observable receiving(client, boost::make_iterator_range(received.data(), received.data() + received.size()));
		auto receiver = rx::make_observable_source(rx::ref(receiving), yield);
		boost::optional<Si::http::request_header> request = Si::http::parse_header(receiver);
		if (!request)
		{
			return;
		}
		std::vector<char> send_buffer;
		{
			auto response_sink = Si::make_container_sink(send_buffer);
			Si::http::response_header response;
			response.http_version = "HTTP/1.0";
			response.status = 200;
			response.status_text = "OK";
			auto body = boost::str(boost::format("Hello, visitor %1%") % visitor_number);
			response.arguments["Content-Length"] = boost::lexical_cast<std::string>(body.size());
			Si::http::write_header(response_sink, response);
			Si::append(response_sink, body);
		}
		rx::sending_observable sending(
			client,
			boost::make_iterator_range(send_buffer.data(), send_buffer.data() + send_buffer.size()));
		while (yield.get_one((sending)))
		{
		}
	}

	struct accept_handler : boost::static_visitor<events>
	{
		boost::uintmax_t visitor_number;

		explicit accept_handler(boost::uintmax_t visitor_number)
			: visitor_number(visitor_number)
		{
		}

		events operator()(std::shared_ptr<boost::asio::ip::tcp::socket> client) const
		{
			auto visitor_number_ = visitor_number;
			auto client_handler = rx::wrap<rx::detail::nothing>(rx::make_coroutine<rx::detail::nothing>([client, visitor_number_](rx::yield_context<rx::detail::nothing> &yield) -> void
			{
				return serve_client(*client, yield, visitor_number_);
			}));
			return client_handler;
		}

		events operator()(boost::system::error_code) const
		{
			throw std::logic_error("not implemented");
		}
	};
}

int main()
{
	boost::asio::io_service io;
	boost::asio::ip::tcp::acceptor acceptor(io, boost::asio::ip::tcp::endpoint(boost::asio::ip::address_v4(), 8080));
	rx::tcp_acceptor clients(acceptor);
	auto handling_clients = rx::flatten(rx::make_coroutine<events>([&clients](rx::yield_context<events> &yield) -> void
	{
		auto visitor_counter = rx::make_finite_state_machine(
									rx::generate([]() { return rx::nothing{}; }),
									static_cast<boost::uintmax_t>(0),
									[](boost::uintmax_t previous, rx::nothing) { return previous + 1; });
		for (;;)
		{
			auto result = yield.get_one(clients);
			if (!result)
			{
				break;
			}
			auto const visitor_number = yield.get_one(visitor_counter);
			assert(visitor_number);
			accept_handler handler{*visitor_number};
			auto context = Si::apply_visitor(handler, *result);
			if (!context.empty())
			{
				yield(std::move(context));
			}
		}
	}));
	auto all = rx::make_total_consumer(rx::ref(handling_clients));
	all.start();
	io.run();
}
