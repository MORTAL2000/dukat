#include "stdafx.h"
#include <dukat/log.h>
#include <dukat/settings.h>

#ifndef __ANDROID__
#include <spdlog/spdlog.h>
#include <spdlog/sinks/file_sinks.h>
#include <spdlog/sinks/null_sink.h>
#include <spdlog/sinks/stdout_sinks.h>
#endif

namespace dukat
{
#ifndef __ANDROID__
	std::shared_ptr<spdlog::logger> log = spdlog::stdout_logger_st("default");
#else
	std::shared_ptr<Logger> log = std::shared_ptr<Logger>();
#endif

	void init_logging(const Settings& settings)
	{
#ifndef __ANDROID__
		spdlog::set_pattern("%^[%Y-%m-%d %H-%M-%S.%e] [%l] %v%$");
		// Enable async flush every 2 seconds
		spdlog::set_async_mode(4096, spdlog::async_overflow_policy::block_retry,
                       nullptr,
                       std::chrono::seconds(2));

		const auto log_output = settings.get_string("logging.output", "console");
		if (log_output == "console")
		{
			log = spdlog::stdout_color_st("console");
			auto console_sink = dynamic_cast<spdlog::sinks::wincolor_stdout_sink_st*>(log->sinks().back().get());
#ifdef WIN32
			console_sink->set_color(spdlog::level::trace, console_sink->CYAN);
			console_sink->set_color(spdlog::level::debug, console_sink->BOLD);
			console_sink->set_color(spdlog::level::info, console_sink->WHITE);
			console_sink->set_color(spdlog::level::warn, console_sink->YELLOW);
			console_sink->set_color(spdlog::level::err, console_sink->RED);
#else
			// TODO: Fixme
			// console_sink->set_color(spdlog::level::info, "\033[37m");
#endif
		}
		else if (log_output == "file")
		{
			const auto log_file = settings.get_string("logging.file", "run.log");
			log = spdlog::create<spdlog::sinks::simple_file_sink_st>("file_logger", log_file);
		}
		else
		{
			log = spdlog::create<spdlog::sinks::null_sink_st>("null_logger");
		}

		const auto log_level =  settings.get_string("logging.level", "debug");
		spdlog::level::level_enum level;
		if (log_level == "error")
			level = spdlog::level::err;
		else if (log_level == "warn")
			level = spdlog::level::warn;
		else if (log_level == "info")
			level = spdlog::level::info;
		else if (log_level == "debug")
			level = spdlog::level::debug;
		else if (log_level == "trace")
			level = spdlog::level::trace;
		else
			throw std::runtime_error("Invalid log level.");
		log->set_level(level);
#endif
	}
}
