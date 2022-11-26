#include "Log.h"

#include "utils/FileSystemUtil.h"
#include "platform.h"
#include <iostream>
#include <mutex>
#include "Settings.h"
#include <iomanip> 
#include <chrono>

static std::mutex mLogLock;

LogLevel Log::reportingLevel = LogInfo;
bool Log::dirty = false;
FILE* Log::file = NULL;

LogLevel Log::getReportingLevel()
{
	return reportingLevel;
}

std::string Log::getLogPath()
{
	return Utils::FileSystem::getEsConfigPath() + "/es_log.txt";
}

void Log::setReportingLevel(LogLevel level)
{
	reportingLevel = level;
}

void Log::init()
{
	std::unique_lock<std::mutex> lock(mLogLock);

	if (file != NULL)
		close();

	if (Settings::getInstance()->getString("LogLevel") == "disabled")
	{
		remove(getLogPath().c_str());
		return;
	}

	remove((getLogPath() + ".bak").c_str());

	// rename previous log file
	rename(getLogPath().c_str(), (getLogPath() + ".bak").c_str());

	file = fopen(getLogPath().c_str(), "w");
	dirty = false;
}

std::ostringstream& Log::get(LogLevel level)
{
	//time_t t = time(nullptr);
	//os << std::put_time(localtime(&t), "%F %T\t");

	std::chrono::system_clock::time_point tp = std::chrono::system_clock::now();

	time_t raw_time = std::chrono::system_clock::to_time_t(tp);

		// Tm* does not need to be deleted after use, because tm* is created by localtime, and there will be one in each thread
	struct tm  *timeinfo = std::localtime(&raw_time);

	os << std::put_time(timeinfo, "%F %T");

	if (Settings::getInstance()->getBool("LogWithMilliseconds"))
	{
			// tm can only go to seconds, milliseconds need to be obtained separately
		std::chrono::milliseconds ms = std::chrono::duration_cast<std::chrono::milliseconds>(tp.time_since_epoch());

		std::string milliseconds_str = std::to_string(ms.count() % 1000);

		if (milliseconds_str.length() < 3)
			milliseconds_str = (std::string(3 - milliseconds_str.length(), '0') + milliseconds_str).c_str();

		os << ':' << milliseconds_str;
	}

	os << '\t';

	switch (level)
	{
	case LogError:
		os << "ERROR\t";
		break;
	case LogWarning:
		os << "WARNING\t";
		break;
	case LogDebug:
		os << "DEBUG\t";
		break;
	default:
		os << "INFO\t";
		break;
	}

	messageLevel = level;

	return os;
}

void Log::flush()
{
	if (!dirty)
		return;

	if (file != nullptr)
		fflush(file);

	dirty = false;
}

void Log::close()
{
	if (file != NULL)
	{
		fflush(file);
		fclose(file);
	}

	dirty = false;
	file = NULL;
}

Log::~Log()
{
	std::unique_lock<std::mutex> lock(mLogLock);

	if (file != NULL)
	{
		os << std::endl;
		fprintf(file, "%s", os.str().c_str());
		dirty = true;
	}

	// If it's an error, also print to console
	// print all messages if using --debug
	if (messageLevel == LogError || reportingLevel >= LogDebug)
	{
		fprintf(stderr, "%s", os.str().c_str());
	}
}

void Log::setupReportingLevel()
{
	LogLevel lvl = LogInfo;

	if (Settings::getInstance()->getBool("Debug"))
		lvl = LogDebug;
	else
	{
		auto level = Settings::getInstance()->getString("LogLevel");
		if (level == "debug")
			lvl = LogDebug;
		else if (level == "information")
			lvl = LogInfo;
		else if (level == "warning")
			lvl = LogWarning;
		else if (level == "error")
			lvl = LogError;
	}

	setReportingLevel(lvl);
}

bool Log::isErrorLevel()
{
	return reportingLevel == LogError;
}

bool Log::isWarningLevel()
{
	return reportingLevel == LogWarning;
}

bool Log::isInfoLevel()
{
	return reportingLevel == LogInfo;
}

bool Log::isDebugLevel()
{
	return reportingLevel == LogDebug;
}
