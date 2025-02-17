#include "utils/TimeUtil.h"
#include <time.h>
#include "EsLocale.h"

namespace Utils
{
	namespace Time
	{
		DateTime DateTime::now()
		{
			return Utils::Time::DateTime(Utils::Time::now());
		}

		DateTime::DateTime()
		{
			mTime       = 0;
			mTimeStruct = { 0, 0, 0, 1, 0, 0, 0, 0, -1 };
			mIsoString  = "00000000T000000";

		} // DateTime::DateTime

		DateTime::DateTime(const time_t& _time)
		{
			setTime(_time);

		} // DateTime::DateTime

		DateTime::DateTime(const tm& _timeStruct)
		{
			setTimeStruct(_timeStruct);

		} // DateTime::DateTime

		DateTime::DateTime(const std::string& _isoString)
		{
			setIsoString(_isoString);

		} // DateTime::DateTime

		DateTime::~DateTime()
		{

		} // DateTime::~DateTime

		void DateTime::setTime(const time_t& _time)
		{
			try
			{
				mTime = (_time < 0) ? 0 : _time;
				mTimeStruct = *localtime(&mTime);
				mIsoString = timeToString(mTime);
			}
			catch (...)
			{
				mTime = 0;
				mTimeStruct = { 0, 0, 0, 1, 0, 0, 0, 0, -1 };
				mIsoString = "00000000T000000";
			}
		} // DateTime::setTime

		void DateTime::setTimeStruct(const tm& _timeStruct)
		{
			setTime(mktime((tm*)&_timeStruct));

		} // DateTime::setTimeStruct

		void DateTime::setIsoString(const std::string& _isoString)
		{
			setTime(stringToTime(_isoString));

		} // DateTime::setIsoString

		double	DateTime::elapsedSecondsSince(const DateTime& _since)
		{
			return difftime(mTime, _since.mTime);
		}

		std::string DateTime::toLocalTimeString()
		{
			time_t     clockNow = getTime();
			struct tm  clockTstruct = *localtime(&clockNow);

			char       clockBuf[256];
			strftime(clockBuf, sizeof(clockBuf), "%Ex %R", &clockTstruct);
			return clockBuf;
		}

		Duration::Duration(const time_t& _time)
		{
			mTotalSeconds = (unsigned int)_time;
			mDays         = (mTotalSeconds - (mTotalSeconds % (60*60*24))) / (60*60*24);
			mHours        = ((mTotalSeconds % (60*60*24)) - (mTotalSeconds % (60*60))) / (60*60);
			mMinutes      = ((mTotalSeconds % (60*60)) - (mTotalSeconds % (60))) / 60;
			mSeconds      = mTotalSeconds % 60;

		} // Duration::Duration

		Duration::~Duration()
		{

		} // Duration::~Duration

		time_t now()
		{
			time_t time;
			::time(&time);
			return time;

		} // now

		time_t stringToTime(const std::string& _string, const std::string& _format)
		{
			const char* s           = _string.c_str();
			const char* f           = _format.c_str();
			tm          timeStruct  = { 0, 0, 0, 1, 0, 0, 0, 0, -1 };
			size_t      parsedChars = 0;

			if(_string == "not-a-date-time")
				return mktime(&timeStruct);

			while(*f && (parsedChars < _string.length()))
			{
				if(*f == '%')
				{
					++f;

					switch(*f++)
					{
						case 'Y': // The year [1970,xxxx]
						{
							if((parsedChars + 4) <= _string.length())
							{
								timeStruct.tm_year  = (*s++ - '0') * 1000;
								timeStruct.tm_year += (*s++ - '0') * 100;
								timeStruct.tm_year += (*s++ - '0') * 10;
								timeStruct.tm_year += (*s++ - '0');
								if(timeStruct.tm_year >= 1900)
									timeStruct.tm_year -= 1900;
							}

							parsedChars += 4;
						}
						break;

						case 'm': // The month number [01,12]
						{
							if((parsedChars + 2) <= _string.length())
							{
								timeStruct.tm_mon  = (*s++ - '0') * 10;
								timeStruct.tm_mon += (*s++ - '0');
								if(timeStruct.tm_mon >= 1)
									timeStruct.tm_mon -= 1;
							}

							parsedChars += 2;
						}
						break;

						case 'd': // The day of the month [01,31]
						{
							if((parsedChars + 2) <= _string.length())
							{
								timeStruct.tm_mday  = (*s++ - '0') * 10;
								timeStruct.tm_mday += (*s++ - '0');
							}

							parsedChars += 2;
						}
						break;

						case 'H': // The hour (24-hour clock) [00,23]
						{
							if((parsedChars + 2) <= _string.length())
							{
								timeStruct.tm_hour  = (*s++ - '0') * 10;
								timeStruct.tm_hour += (*s++ - '0');
							}

							parsedChars += 2;
						}
						break;

						case 'M': // The minute [00,59]
						{
							if((parsedChars + 2) <= _string.length())
							{
								timeStruct.tm_min  = (*s++ - '0') * 10;
								timeStruct.tm_min += (*s++ - '0');
							}

							parsedChars += 2;
						}
						break;

						case 'S': // The second [00,59]
						{
							if((parsedChars + 2) <= _string.length())
							{
								timeStruct.tm_sec  = (*s++ - '0') * 10;
								timeStruct.tm_sec += (*s++ - '0');
							}

							parsedChars += 2;
						}
						break;
					}
				}
				else
				{
					++s;
					++f;
				}
			}

			return mktime(&timeStruct);

		} // stringToTime

		std::string timeToString(const time_t& _time, const std::string& _format)
		{
			const char* f          = _format.c_str();
			const tm    timeStruct = *localtime(&_time);
			char        buf[256]   = { '\0' };
			const int   MAX_LENGTH = 256;

			// Use strftime to format the string
			if (!strftime(buf, MAX_LENGTH, _format.c_str(), &timeStruct)) {
				return "";
			}
			else 
			{
				return std::string(buf);
			}
		} // timeToString

		  // transforms a number of seconds into a human readable string
		std::string secondsToString(const long seconds)
		{
			if (seconds == 0)
				return _("never");

			char buf[256];

			int h = 0, m = 0, s = 0;
			h = (seconds / 3600);
			m = (seconds / 60) % 60;
			s = seconds % 60;

			if (h > 0)
			{
				snprintf(buf, 256, _("%d h").c_str(), h);
				if (m > 0)
				{
					std::string hours(buf);
					snprintf(buf, 256, _("%d m").c_str(), m);
					return hours + " " + std::string(buf);
				}
			}
			else if (m > 0)
				snprintf(buf, 256, _("%d m").c_str(), m);
			else
				snprintf(buf, 256, _("%d s").c_str(), s);

			return std::string(buf);
		}

		int daysInMonth(const int _year, const int _month)
		{
			tm timeStruct = { 0, 0, 0, 0, _month, _year - 1900, 0, 0, -1 };
			mktime(&timeStruct);

			return timeStruct.tm_mday;

		} // daysInMonth

		int daysInYear(const int _year)
		{
			tm timeStruct = { 0, 0, 0, 0, 0, _year - 1900 + 1, 0, 0, -1 };
			mktime(&timeStruct);

			return timeStruct.tm_yday + 1;

		} // daysInYear

	} // Time::

} // Utils::
