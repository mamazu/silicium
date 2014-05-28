#include <silicium/git/repository.hpp>
#include <git2/errors.h>
#include <array>

namespace Si
{
	namespace git
	{
		void repository_deleter::operator()(git_repository *repository)
		{
			git_repository_free(repository);
		}

		void reference_deleter::operator()(git_reference *reference)
		{
			git_reference_free(reference);
		}

		void commit_deleter::operator()(git_commit *commit)
		{
			git_commit_free(commit);
		}

		git_error::git_error(int code, std::string message)
			: std::runtime_error(std::move(message))
			, m_code(code)
		{
		}

		int git_error::code() const
		{
			return m_code;
		}

		void throw_if_libgit2_error(int error)
		{
			if (error)
			{
				auto * const details = giterr_last();
				if (details)
				{
					throw git_error(error, details->message);
				}
				else
				{
					throw git_error(error, "");
				}
			}
		}

		unique_repository open_repository(boost::filesystem::path const &where)
		{
			git_repository *repo = nullptr;
			throw_if_libgit2_error(git_repository_open(&repo, where.string().c_str()));
			return unique_repository(repo);
		}

		unique_repository clone(std::string const &source, boost::filesystem::path const &destination, git_clone_options const *options)
		{
			git_repository *repo = nullptr;
			throw_if_libgit2_error(git_clone(&repo, source.c_str(), destination.string().c_str(), options));
			return unique_repository(repo);
		}

		unique_reference lookup(git_repository &repository, char const *name)
		{
			git_reference *ref = nullptr;
			auto const error = git_reference_lookup(&ref, &repository, name);
			if (error == GIT_ENOTFOUND)
			{
				return nullptr;
			}
			throw_if_libgit2_error(error);
			return unique_reference(ref);
		}

		unique_commit lookup_commit(git_repository &repository, git_oid const &id)
		{
			git_commit *commit = nullptr;
			auto const error = git_commit_lookup(&commit, &repository, &id);
			if (error == GIT_ENOTFOUND)
			{
				return nullptr;
			}
			throw_if_libgit2_error(error);
			return unique_commit(commit);
		}

		std::string format_oid(git_oid const &id)
		{
			std::array<char, 41> str;
			git_oid_fmt(str.data(), &id);
			str[40] = 0;
			return str.data();
		}
	}
}
