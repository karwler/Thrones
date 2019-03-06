#include "fileSys.h"
#ifndef _WIN32
#include <unistd.h>
#endif

// SETTINGS

const array<string, Settings::vsyncNames.size()> Settings::vsyncNames = {
	"immediate",
	"synchronized",
	"adaptive"
};

Settings::Settings() :
	maximized(false),
	fullscreen(false),
	vsync(VSync::synchronized),
	resolution(800, 600),
	address("127.0.0.1"),
	port(Server::defaultPort)
{}

// FILE SYS

FileSys::FileSys() {
	setWorkingDir();

	// check if all (more or less) necessary files and directories exist
	if (fileType(dirSavs) != FTYPE_DIR && !createDir(dirSavs))
		std::cerr << "couldn't create save data directory" << std::endl;
	if (fileType(dirTexs) != FTYPE_DIR)
		std::cerr << "couldn't find texture directory" << std::endl;
}

Settings* FileSys::loadSettings() {
	Settings* sets = new Settings();
	for (const string& line : readFileLines(string(dirSavs) + fileSettings)) {
		if (pairStr il = splitIniLine(line); il.first == iniKeywordMaximized)
			sets->maximized = stob(il.second);
		else if (il.first == iniKeywordFullscreen)
			sets->fullscreen = stob(il.second);
		else if (il.first == iniKeywordResolution)
			sets->resolution = vec2i::get(il.second, strtoul, 0);
		else if (il.first == iniKeywordVsync)
			sets->vsync = strToEnum<Settings::VSync>(Settings::vsyncNames, il.second);
		else if (il.first == iniKeywordAddress)
			sets->address = il.second;
		else if (il.first == iniKeywordPort)
			sets->port = uint16(stoul(il.second));
	}
	return sets;
}

bool FileSys::saveSettings(const Settings* sets) {
	string text;
	text += makeIniLine(iniKeywordMaximized, btos(sets->maximized));
	text += makeIniLine(iniKeywordFullscreen, btos(sets->fullscreen));
	text += makeIniLine(iniKeywordResolution, sets->resolution.toString());
	text += makeIniLine(iniKeywordVsync, Settings::vsyncNames[uint8(sets->vsync)]);
	text += makeIniLine(iniKeywordAddress, sets->address);
	text += makeIniLine(iniKeywordPort, to_string(sets->port));
	return writeTextFile(string(dirSavs) + fileSettings, text);
}

vector<string> FileSys::readFileLines(const string& file, bool printMessage) {
	vector<string> lines(1);
	for (char ch : readTextFile(file, printMessage)) {
		if (ch != '\n' && ch != '\r')
			lines.back() += ch;
		else if (!lines.back().empty())
			lines.push_back(emptyStr);
	}
	if (lines.back().empty())
		lines.pop_back();
	return lines;
}

string FileSys::readTextFile(const string& file, bool printMessage) {
	FILE* ifh = fopen(file.c_str(), "rb");
	if (!ifh) {
		if (printMessage)
			std::cerr << "couldn't open file " << file << std::endl;
		return "";
	}
	fseek(ifh, 0, SEEK_END);
	sizet len = sizet(ftell(ifh));
	fseek(ifh, 0, SEEK_SET);

	string text;
	text.resize(len);
	fread(text.data(), sizeof(char), text.length(), ifh);

	fclose(ifh);
	return text;
}

bool FileSys::writeTextFile(const string& file, const string& text) {
	FILE* ofh = fopen(file.c_str(), "wb");
	if (!ofh) {
		std::cerr << "couldn't write file " << file << std::endl;
		return false;
	}
	fputs(text.c_str(), ofh);
	return !fclose(ofh);
}

pairStr FileSys::splitIniLine(const string& line) {
	sizet id = line.find_first_of('=');
	return id != string::npos ? pair(trim(line.substr(0, id)), trim(line.substr(id + 1))) : pairStr();
}

vector<string> FileSys::listDir(const string& drc, FileType filter) {
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	HANDLE hFind = FindFirstFileW(stow(appDsep(drc) + "*").c_str(), &data);
	if (hFind == INVALID_HANDLE_VALUE)
		return entries;

	do {
		if (!isDotName(data.cFileName) && atrcmp(data.dwFileAttributes, filter))
			entries.emplace_back(wtos(data.cFileName));
	} while (FindNextFileW(hFind, &data));
	FindClose(hFind);
#else
	DIR* directory = opendir(drc.c_str());
	if (!directory)
		return entries;

	while (dirent* entry = readdir(directory))
		if (!isDotName(entry->d_name) && dtycmp(drc, entry, filter, true))
			entries.emplace_back(entry->d_name);
	closedir(directory);
#endif
	std::sort(entries.begin(), entries.end());
	return entries;
}

void FileSys::setWorkingDir() {
#ifdef _WIN32
	wchar* buf = new wchar[MAX_PATH];
	DWORD len = GetModuleFileNameW(nullptr, buf, MAX_PATH);
	if (!len || len == MAX_PATH) {
		delete[] buf;
		buf = new wchar[pathMax];
		len = GetModuleFileNameW(nullptr, buf, pathMax);
	}
	while (len > 0 && buf[--len] != dsep);	// terminate path stirng at last dsep
	buf[len] = '\0';
	if (!len || _wchdir(buf))
		std::cerr << "Couldn't set working directory" << std::endl;
#else
	char* buf = new char[PATH_MAX];
	if (sizet len = sizet(readlink(linkExe, buf, PATH_MAX)); len > PATH_MAX || chdir(parentPath(string(buf, buf + len)).c_str()))
		std::cerr << "couldn't set working directory" << std::endl;
#endif
	delete[] buf;
}
#ifdef _WIN32
string FileSys::wgetenv(const string& name) {
	wstring var = stow(name);
	DWORD len = GetEnvironmentVariableW(var.c_str(), nullptr, 0);
	if (len <= 1)
		return emptyStr;

	wstring str;
	str.resize(len - 1);
	GetEnvironmentVariableW(var.c_str(), str.data(), len);
	return wtos(str);
}

FileType FileSys::fileType(const string& file, bool readLink) {
	DWORD attrib = GetFileAttributesW(stow(file).c_str());
	if (attrib == INVALID_FILE_ATTRIBUTES)
		return FTYPE_NON;
	if (attrib & FILE_ATTRIBUTE_DIRECTORY)
		return FTYPE_DIR;
	return FTYPE_REG;
}
#else
FileType FileSys::stmtoft(const string& file, int (*statfunc)(const char*, struct stat*)) {
	struct stat ps;
	if (statfunc(file.c_str(), &ps))
		return FTYPE_NON;

	switch (ps.st_mode & S_IFMT) {
	case S_IFDIR:
		return FTYPE_DIR;
	case S_IFREG:
		return FTYPE_REG;
	}
	return FTYPE_OTH;
}

bool FileSys::dtycmp(const string& drc, const dirent* entry, FileType filter, bool readLink) {
	switch (entry->d_type) {
	case DT_DIR:
		return filter & FTYPE_DIR;
	case DT_REG:
		return filter & FTYPE_REG;
	case DT_LNK:
		return filter & (readLink ? stmtoft(drc + entry->d_name, stat) : FTYPE_OTH);
	case DT_UNKNOWN:
		return filter & stmtoft(drc + entry->d_name, readLink ? stat : lstat);
	}
	return filter & FTYPE_OTH;
}
#endif
