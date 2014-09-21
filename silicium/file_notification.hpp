#ifndef SILICIUM_REACTIVE_FILE_NOTIFICATION_HPP
#define SILICIUM_REACTIVE_FILE_NOTIFICATION_HPP

#include <boost/detail/scoped_enum_emulation.hpp>
#include <boost/filesystem/path.hpp>

namespace Si
{
	BOOST_SCOPED_ENUM_DECLARE_BEGIN(file_notification_type)
	{
		add,
		remove,
		change
	}
	BOOST_SCOPED_ENUM_DECLARE_END(file_notification_type)

	struct file_notification
	{
		file_notification_type type;
		boost::filesystem::path name;

		file_notification()
			: type(file_notification_type::change)
		{
		}

		file_notification(file_notification_type type, boost::filesystem::path name)
			: type(type)
			, name(std::move(name))
		{
		}
	};
}

#endif
