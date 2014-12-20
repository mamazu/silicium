#ifndef SILICIUM_LINUX_PROCESS_HPP
#define SILICIUM_LINUX_PROCESS_HPP

#include <silicium/sink/ptr_sink.hpp>
#include <silicium/sink/buffering_sink.hpp>
#include <silicium/process_parameters.hpp>
#include <silicium/file_handle.hpp>
#include <boost/filesystem/operations.hpp>
#include <fcntl.h>
#include <sys/wait.h>
#include <sys/prctl.h>

namespace Si
{
	namespace detail
	{
		template <class Error>
		inline void copy_all(int source, sink<char, Error> &destination)
		{
			for (;;)
			{
				auto buffer_helper = make_buffering_sink(ref_sink(destination));
				auto const buffer = buffer_helper.make_append_space(std::numeric_limits<std::size_t>::max());
				assert(!buffer.empty());
				auto const rc = read(source, buffer.begin(), buffer.size());
				if (rc == 0)
				{
					break;
				}
				if (rc < 0)
				{
					throw boost::system::system_error(errno, boost::system::system_category());
				}
				buffer_helper.make_append_space(static_cast<std::size_t>(rc));
				buffer_helper.flush_append_space();
			}
		}

		inline void set_close_on_exec(int file)
		{
			fcntl(file, F_SETFD, fcntl(file, F_GETFD) | FD_CLOEXEC);
		}

		struct pipe
		{
			file_handle write, read;

			void close() BOOST_NOEXCEPT
			{
				pipe().swap(*this);
			}

			void swap(pipe &other) BOOST_NOEXCEPT
			{
				write.swap(other.write);
				read.swap(other.read);
			}
		};

		inline pipe make_pipe()
		{
			std::array<int, 2> fds;
			if (::pipe(fds.data()) < 0)
			{
				throw boost::system::system_error(errno, boost::system::system_category());
			}
			pipe result;
			result.read  = file_handle(fds[0]);
			result.write = file_handle(fds[1]);
			return result;
		}
	}

	inline int run_process(process_parameters const &parameters)
	{
		auto executable = parameters.executable.string();
		auto arguments = parameters.arguments;
		std::vector<char *> argument_pointers;
		argument_pointers.emplace_back(const_cast<char *>(executable.c_str()));
		std::transform(begin(arguments), end(arguments), std::back_inserter(argument_pointers), [](std::string &arg)
		{
			return &arg[0];
		});
		argument_pointers.emplace_back(nullptr);

		detail::pipe stdout;
		if (parameters.out)
		{
			stdout = detail::make_pipe();
		}

		auto stdin = detail::make_pipe();
		auto child_error = detail::make_pipe();

		pid_t const forked = fork();
		if (forked < 0)
		{
			throw boost::system::system_error(errno, boost::system::system_category());
		}

		//child
		if (forked == 0)
		{
			auto const fail = [&child_error]()
#if defined(__GNUC__) && !defined(__clang__)
				__attribute__ ((__noreturn__))
#endif
			{
				int error = errno;
				ssize_t written = write(child_error.write.handle, &error, sizeof(error));
				if (written != sizeof(error))
				{
					_exit(1);
				}
				child_error.write.close();
				_exit(0);
			};

			if (parameters.out)
			{
				if (dup2(stdout.write.handle, STDOUT_FILENO) < 0)
				{
					fail();
				}
				if (dup2(stdout.write.handle, STDERR_FILENO) < 0)
				{
					fail();
				}
				stdout.close();
			}
			else
			{
				//drop output
				dup2(open("/dev/null", O_RDWR), STDOUT_FILENO);
				dup2(open("/dev/null", O_RDWR), STDERR_FILENO);
			}

			if (dup2(stdin.read.handle, STDIN_FILENO) < 0)
			{
				fail();
			}
			else
			{
				//provide empty input
				dup2(open("/dev/null", O_RDWR), STDIN_FILENO);
			}
			stdin.close();

			child_error.read.close();
			detail::set_close_on_exec(child_error.write.handle);

			boost::filesystem::current_path(parameters.current_path);

			//close inherited file descriptors
			long max_fd = sysconf(_SC_OPEN_MAX);
			for (int i = 3; i < max_fd; ++i)
			{
				if (i == child_error.write.handle)
				{
					continue;
				}
				close(i);
			}

			//kill the child when the parent exits
			prctl(PR_SET_PDEATHSIG, SIGHUP);

			execvp(parameters.executable.c_str(), argument_pointers.data());
			fail();
		}

		//parent
		else
		{
			child_error.write.close();

			if (parameters.out)
			{
				stdout.write.close();
				detail::copy_all(stdout.read.handle, *parameters.out);
			}
			stdin.read.close();

			int status = 0;
			if (waitpid(forked, &status, 0) < 0)
			{
				throw boost::system::system_error(errno, boost::system::system_category());
			}

			int error = 0;
			ssize_t read_error = read(child_error.read.handle, &error, sizeof(error));
			if (read_error < 0)
			{
				throw boost::system::system_error(errno, boost::system::system_category());
			}
			if (read_error != 0)
			{
				assert(read_error == sizeof(error));
				throw boost::system::system_error(error, boost::system::system_category());
			}

			int const exit_status = WEXITSTATUS(status);
			return exit_status;
		}
	}
}

#endif
