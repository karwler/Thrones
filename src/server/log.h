#pragma once

#include "utils/text.h"
#include <deque>
#include <fstream>

// struct tm wrapper
struct Date {
	uint8 sec, min, hour;
	uint8 day, month;
	uint8 wday;
	int16 year;

	Date(uint8 second, uint8 minute, uint8 dhour, uint8 mday, uint8 ymonth, int16 tyear, uint8 weekDay);

	static Date now();
	string toString(char ts = '-', char sep = '_', char ds = '-') const;
};

inline string Date::toString(char ts, char sep, char ds) const {
	return toStr(year) + ds + toStr(month, 2) + ds + toStr(day, 2) + sep + toStr(hour, 2) + ts + toStr(min, 2) + ts + toStr(sec, 2);
}

// for simultaneous console and file output
class Log {
private:
	static constexpr uint maxLogfiles = 8;
	static constexpr char fileRecords[] = "records.txt";

	string dir;	// TODO: handle unicode on Windows
	std::deque<string> files;
	std::ofstream lfile;
	uint8 lastDay;
	bool verbose;

public:
	void start(bool logStd, const char* logDir);
	void end();

	template <class... A> void write(std::ostream& vofs, A&&... args);
private:
	void openFile(const Date& now);
	void removeOldFiles();
	vector<string> readFile(const string& path);
};

template <class... A>
void Log::write(std::ostream& vofs, A&&... args) {
	if (verbose)
		(vofs << ... << std::forward<A>(args)) << std::endl;
	if (!dir.empty()) {
		if (lfile.good())
			(lfile << ... << std::forward<A>(args)) << linend;
		if (Date now = Date::now(); now.day != lastDay) {
			lfile.close();
			removeOldFiles();
			openFile(now);
		}
	}
}
