#include <reactive/coroutine.hpp>
#include <silicium/http/http.hpp>
#include <silicium/fast_variant.hpp>
#include <reactive/total_consumer.hpp>
#include <reactive/consume.hpp>
#include <boost/asio.hpp>
#include <sys/inotify.h>

namespace rx
{
	struct file_system_change
	{
		boost::filesystem::path name;
	};

	struct watch_descriptor
	{
		watch_descriptor()
		{
		}

		watch_descriptor(int notifier, int watch)
			: notifier(notifier)
			, watch(watch)
		{
		}

	private:

		int notifier;
		int watch;
	};

	struct file_system_notifier : observable<std::vector<file_system_change>>
	{
		typedef std::vector<file_system_change> element_type;

		explicit file_system_notifier(boost::asio::io_service &io)
			: notifier(io)
		{
			int fd = inotify_init();
			if (fd < 0)
			{
				throw boost::system::system_error(errno, boost::system::posix_category);
			}
			try
			{
				notifier.assign(fd);
			}
			catch (...)
			{
				close(fd);
				throw;
			}
		}

		~file_system_notifier()
		{
			close(notifier.native_handle());
		}

		watch_descriptor watch(boost::filesystem::path const &target)
		{
			boost::uint32_t flags = IN_CREATE | IN_DELETE;
			int wd = inotify_add_watch(notifier.native_handle(), target.string().c_str(), flags);
			if (wd < 0)
			{
				throw boost::system::system_error(errno, boost::system::posix_category);
			}
			return watch_descriptor(notifier.native_handle(), wd);
		}

		virtual void async_get_one(observer<element_type> &receiver) SILICIUM_OVERRIDE
		{
			assert(!receiver_);
			read_buffer.resize(8192);
			notifier.async_read_some(boost::asio::buffer(read_buffer), [this](boost::system::error_code error, std::size_t bytes_read)
			{
				if (error)
				{
					if (error == boost::asio::error::operation_aborted)
					{
						return;
					}
					throw std::logic_error("not implemented");
				}
				else
				{
					std::vector<file_system_change> changes;
					for (std::size_t i = 0; i < bytes_read; )
					{
						inotify_event const &event = *reinterpret_cast<inotify_event const *>(read_buffer.data() + i);
						changes.emplace_back(file_system_change{event.name});
						i += sizeof(inotify_event);
						i += event.len;
					}
					exchange(this->receiver_, nullptr)->got_element(std::move(changes));
				}
			});
			receiver_ = &receiver;
		}

		virtual void cancel() SILICIUM_OVERRIDE
		{
			assert(receiver_);
			notifier.cancel();
			receiver_ = nullptr;
		}

	private:

		boost::asio::posix::stream_descriptor notifier;
		std::vector<char> read_buffer;
		observer<element_type> *receiver_ = nullptr;
	};
}

namespace
{
}

int main()
{
	boost::asio::io_service io;
	rx::file_system_notifier notifier(io);
	auto w = notifier.watch("/home/virtual/dev");
	auto printer = rx::transform(rx::ref(notifier), [](rx::file_system_notifier::element_type const &events) -> rx::detail::nothing
	{
		for (auto const &event : events)
		{
			std::cerr << event.name << '\n';
		}
		return rx::detail::nothing{};
	});
	auto all = rx::make_total_consumer(printer);
	all.start();
	io.run();
}
