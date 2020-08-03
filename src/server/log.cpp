#include "log.h"
#include <ctime>
#ifdef _WIN32
#include <filesystem>
#endif

// DATE

DateTime::DateTime(uint8 second, uint8 minute, uint8 dhour, uint8 mday, uint8 ymonth, uint16 tyear, uint8 weekDay) :
	sec(second),
	min(minute),
	hour(dhour),
	day(mday),
	month(ymonth),
	wday(weekDay),
	year(tyear)
{}

DateTime DateTime::now() {
	time_t rawt = time(nullptr);
	struct tm* tim = localtime(&rawt);
	return DateTime(uint8(tim->tm_sec), uint8(tim->tm_min), uint8(tim->tm_hour), uint8(tim->tm_mday), uint8(tim->tm_mon + 1), uint16(tim->tm_year + 1900), uint8(tim->tm_wday ? tim->tm_wday : 7));
}

// LOG

void Log::start(bool logStd, const char* logDir) {
	verbose = logStd;
	if (logDir && *logDir) {
		if (dir = logDir; !isDsep(dir.back()))
			dir += '/';
		createDirectories(dir);

		for (string& it : readFile(dir + fileRecords))
			files.push_back(std::move(it));
		openFile(DateTime::now());
	}
}

void Log::openFile(const DateTime& now) {
	string name = "thrones_log_" + now.toString() + ".txt";
#ifdef _WIN32
	if (lfile.open(std::filesystem::u8path(dir + name), std::ios::out | std::ios::binary); !lfile.good()) {
#else
	if (lfile.open(dir + name, std::ios::out | std::ios::binary); !lfile.good()) {
#endif
		if (verbose)
			std::cerr << "failed to write log file " << dir + name << std::endl;
		return;
	}
	lastLog = now;

	for (files.push_back(std::move(name)); files.size() > maxLogfiles; files.pop_front())
#ifdef _WIN32
		_wremove(stow(dir + files.front()).c_str());
#else
		remove((dir + files.front()).c_str());
#endif
#ifdef _WIN32
	if (FILE* ofh = _wfopen(stow(dir + fileRecords).c_str(), L"wb")) {
#else
	if (FILE* ofh = fopen((dir + fileRecords).c_str(), defaultWriteMode)) {
#endif
		for (const string& it : files) {
			fputs(it.c_str(), ofh);
			fputs(linend, ofh);
		}
		fclose(ofh);
	}
}

vector<string> Log::readFile(const string& path) {
	string text;
#ifdef _WIN32
	if (FILE* ifh = _wfopen(stow(path).c_str(), L"rb")) {
#else
	if (FILE* ifh = fopen(path.c_str(), defaultReadMode)) {
#endif
		try {
			if (fseek(ifh, 0, SEEK_END))
				throw 0;
			long len = ftell(ifh);
			if (len == -1)
				throw 0;
			if (fseek(ifh, 0, SEEK_SET))
				throw 0;

			text.resize(len);
			if (sizet red = fread(text.data(), sizeof(*text.data()), text.length(), ifh); red < text.length())
				text.resize(red);
		} catch (...) {}
		fclose(ifh);
	}
	return readTextLines(text);
}
