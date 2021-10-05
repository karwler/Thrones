#include "log.h"
#include <ctime>
#ifdef _WIN32
#include <windows.h>
#ifndef __MINGW32__
#include <filesystem>
#endif
#else
#include <dirent.h>
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
	return DateTime(tim->tm_sec, tim->tm_min, tim->tm_hour, tim->tm_mday, tim->tm_mon + 1, tim->tm_year + 1900, tim->tm_wday ? tim->tm_wday : 7);
}

// LOG

void Log::start(bool logStd, const char* logDir, uint maxLogs) {
	verbose = logStd;
	maxLogfiles = maxLogs;
	if (logDir && *logDir && maxLogfiles) {
		if (dir = logDir; !isDsep(dir.back()))
			dir += '/';
		createDirectories(dir);
		openFile(DateTime::now());
	}
}

void Log::openFile(const DateTime& now) {
	string name = filePrefix + now.toString() + ".txt";
#if defined(_WIN32) && !defined(__MINGW32__)
	if (lfile.open(std::filesystem::u8path(dir + name), std::ios::out | std::ios::binary); !lfile.good()) {
#else
	if (lfile.open(dir + name, std::ios::out | std::ios::binary); !lfile.good()) {
#endif
		if (verbose)
			std::cerr << "failed to write log file " << dir + name << std::endl;
		return;
	}
	lastLog = now;

	if (vector<string> files = listLogs(); files.size() > maxLogfiles)
		for (sizet i = 0, todel = files.size() - maxLogfiles; i < todel; ++i)
#if defined(_WIN32) && !defined(__MINGW32__)
			_wremove(sstow(dir + files[i]).c_str());
#else
			remove((dir + files[i]).c_str());
#endif
}

vector<string> Log::listLogs() const {
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW(sstow(dir + "*").c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		wstring wprefix = cstow(filePrefix);
		do {
			if (!(_wcsnicmp(data.cFileName, wprefix.c_str(), wprefix.length()) || (data.dwFileAttributes & (FILE_ATTRIBUTE_DIRECTORY | FILE_ATTRIBUTE_REPARSE_POINT))))
				entries.push_back(cwtos(data.cFileName));
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
#else
	if (DIR* directory = opendir(dir.c_str())) {
		while (dirent* entry = readdir(directory))
			if (!strncmp(entry->d_name, filePrefix, strlen(filePrefix)) && entry->d_type == DT_REG)
				entries.emplace_back(entry->d_name);
		closedir(directory);
	}
#endif
	std::sort(entries.begin(), entries.end(), strnatless);
	return entries;
}
