#include "log.h"
#include <ctime>
#ifdef _WIN32
#include <windows.h>
#include <filesystem>
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
	return DateTime(uint8(tim->tm_sec), uint8(tim->tm_min), uint8(tim->tm_hour), uint8(tim->tm_mday), uint8(tim->tm_mon + 1), uint16(tim->tm_year + 1900), uint8(tim->tm_wday ? tim->tm_wday : 7));
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

	if (vector<string> files = listLogs(); files.size() > maxLogfiles)
		for (sizet i = 0, todel = files.size() - maxLogfiles; i < todel; ++i)
#ifdef _WIN32
			_wremove(stow(dir + files[i]).c_str());
#else
			remove((dir + files[i]).c_str());
#endif
}

vector<string> Log::listLogs() const {
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW(stow(dir + "*").c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		wstring wprefix = stow(filePrefix);
		do {
			if (data.dwFileAttributes == FILE_ATTRIBUTE_NORMAL && !_wcsnicmp(data.cFileName, wprefix.c_str(), wprefix.length()))
				entries.push_back(wtos(data.cFileName));
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
#else
	if (DIR* directory = opendir(dir.c_str())) {
		while (dirent* entry = readdir(directory))
			if (entry->d_type == DT_REG && !strncmp(entry->d_name, filePrefix, strlen(filePrefix)))
				entries.emplace_back(entry->d_name);
		closedir(directory);
	}
#endif
	std::sort(entries.begin(), entries.end(), strnatless);
	return entries;
}
