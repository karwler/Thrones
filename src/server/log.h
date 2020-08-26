#pragma once

#include "utils/text.h"
#include <iostream>
#include <fstream>

// struct tm wrapper
struct DateTime {
	uint8 sec, min, hour;
	uint8 day, month;
	uint8 wday;
	uint16 year;

	DateTime() = default;
	DateTime(uint8 second, uint8 minute, uint8 dhour, uint8 mday, uint8 ymonth, uint16 tyear, uint8 weekDay);

	static DateTime now();
	string timeString(char ts = '-') const;
	string dateString(char ds = '-') const;
	string toString(char ts = '-', char sep = '_', char ds = '-') const;
	bool datecmp(const DateTime& date) const;
};

inline string DateTime::timeString(char ts) const {
	return toStr(hour, 2) + ts + toStr(min, 2) + ts + toStr(sec, 2);
}

inline string DateTime::dateString(char ds) const {
	return toStr(year) + ds + toStr(month, 2) + ds + toStr(day, 2);
}

inline string DateTime::toString(char ts, char sep, char ds) const {
	return dateString(ds) + sep + timeString(ts);
}

inline bool DateTime::datecmp(const DateTime& date) const {
	return day == date.day && month == date.month && year == date.year;
}

// for simultaneous console and file output
class Log {
public:
	static constexpr uint defaultMaxLogfiles = 8;
private:
	static constexpr char filePrefix[] = "thrones_log_";

	string dir;
	std::ofstream lfile;
	DateTime lastLog;
	uint maxLogfiles;
	bool verbose;

public:
	void start(bool logStd, const char* logDir, uint maxLogs);
	void end();

	template <class... A> void out(A&&... args);
	template <class... A> void err(A&&... args);
private:
	template <class... A> void write(std::ostream& vofs, A&&... args);
	void openFile(const DateTime& now);
	vector<string> listLogs() const;
};

inline void Log::end() {
	lfile.close();
}

template <class... A>
void Log::out(A&&... args) {
	write(std::cout, std::forward<A>(args)...);
}

template <class... A>
void Log::err(A&&... args) {
	write(std::cerr, std::forward<A>(args)...);
}

template <class... A>
void Log::write(std::ostream& vofs, A&&... args) {
	if (verbose)
		(vofs << ... << args) << std::endl;
	if (!dir.empty()) {
		DateTime now = DateTime::now();
		if (!now.datecmp(lastLog)) {
			lfile.close();
			openFile(now);
		}
		if (lfile.good())
			((lfile << now.timeString() << ' ') << ... << std::forward<A>(args)) << std::endl;
	}
}
