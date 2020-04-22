#include "log.h"
#include <ctime>

// DATE

Date::Date(uint8 second, uint8 minute, uint8 dhour, uint8 mday, uint8 ymonth, int16 tyear, uint8 weekDay) :
	sec(second),
	min(minute),
	hour(dhour),
	day(mday),
	month(ymonth),
	wday(weekDay),
	year(tyear)
{}

Date Date::now() {
	time_t rawt = time(nullptr);
	struct tm* tim = localtime(&rawt);
	return Date(uint8(tim->tm_sec), uint8(tim->tm_min), uint8(tim->tm_hour), uint8(tim->tm_mday), uint8(tim->tm_mon + 1), int16(tim->tm_year + 1900), uint8(tim->tm_wday ? tim->tm_wday : 7));
}

// LOG

void Log::start(bool logStd, const char* logDir) {
	verbose = logStd;
	if (logDir && *logDir) {
		if (dir = logDir; !isDsep(dir.back()))
			dir += '/';
		for (string& it : readFile(dir + fileRecords))
			files.push_back(std::move(it));
		removeOldFiles();
		openFile(Date::now());
	}
}

void Log::end() {
	lfile.close();
	if (!files.empty())
		if (std::ofstream rfile(dir + fileRecords, std::ios::binary); rfile.good())
			for (const string& it : files)
				rfile << it << linend;
}

void Log::openFile(const Date& now) {
	string name = "thrones_log_" + now.toString() + ".txt";
	if (lfile.open(dir + name, std::ios::binary); lfile.good()) {
		lastDay = now.day;
		files.push_back(std::move(name));
	} else if (verbose)
		std::cerr << "failed to write log file " << dir + name << std::endl;
}

void Log::removeOldFiles() {
	for (; files.size() >= maxLogfiles; files.pop_front())
		remove((dir + files.front()).c_str());
}

vector<string> Log::readFile(const string& path) {
	string text;
	try {
		std::ifstream ifh(path, std::ios::binary | std::ios::ate);
		if (!ifh.good())
			throw 0;
		std::streampos len = ifh.tellg();
		if (len == -1)
			throw 0;
		ifh.seekg(0);

		text.resize(len);
		if (ifh.read(text.data(), text.length()); sizet(ifh.gcount()) < text.length())
			text.resize(ifh.gcount());
	} catch (...) {
		return vector<string>();
	}
	return readTextLines(text);
}
