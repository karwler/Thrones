#pragma once

#include "utils/text.h"
#include <iostream>
#include <fstream>

// for simultaneous console and file output
class Log {
public:
	static constexpr uint defaultMaxLogfiles = 8;
private:
	static constexpr string_view filePrefix = "thrones_log_";

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
