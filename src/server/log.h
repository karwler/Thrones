#pragma once

#include "utils/text.h"
#include <deque>
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
	string toString(char ts = '-', char sep = '_', char ds = '-') const;
	bool datecmp(const DateTime& date) const;
};

inline string DateTime::toString(char ts, char sep, char ds) const {
	return toStr(year) + ds + toStr(month, 2) + ds + toStr(day, 2) + sep + toStr(hour, 2) + ts + toStr(min, 2) + ts + toStr(sec, 2);
}

inline bool DateTime::datecmp(const DateTime& date) const {
	return day == date.day && month == date.month && year == date.year;
}

// for simultaneous console and file output
class Log {
private:
	static constexpr uint maxLogfiles = 8;
	static constexpr char fileRecords[] = "records.txt";

	string dir;
	std::deque<string> files;
	std::ofstream lfile;
	DateTime lastLog;
	bool verbose;

public:
	void start(bool logStd, const char* logDir);
	void end();

	template <class... A> void write(std::ostream& vofs, A&&... args);
private:
	void openFile(const DateTime& now);
	vector<string> readFile(const string& path);
};

inline void Log::end() {
	lfile.close();
}

template <class... A>
void Log::write(std::ostream& vofs, A&&... args) {
	if (verbose)
		(vofs << ... << std::forward<A>(args)) << std::endl;
	if (!dir.empty()) {
		if (DateTime now = DateTime::now(); !now.datecmp(lastLog)) {
			lfile.close();
			openFile(now);
		}
		if (lfile.good())
			(lfile << ... << std::forward<A>(args)) << std::endl;
	}
}
