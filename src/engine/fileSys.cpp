#include "fileSys.h"
#include <map>
#ifndef _WIN32
#include <unistd.h>
#endif

// SETTINGS

const array<string, Settings::screenNames.size()> Settings::screenNames = {
	"window",
	"borderless",
	"fullscreen",
	"desktop"
};

const array<string, Settings::vsyncNames.size()> Settings::vsyncNames = {
	"adaptive",
	"immediate",
	"synchronized"
};

const array<string, Settings::smoothNames.size()> Settings::smoothNames = {
	"off",
	"fast",
	"nice"
};

Settings::Settings() :
	maximized(false),
	screen(Screen::window),
	vsync(VSync::synchronized),
	samples(4),
	smooth(Smooth::nice),
	size(800, 600),
	mode({ SDL_PIXELFORMAT_RGB888, 1920, 1080, 60, nullptr }),
	address(loopback),
	port(Com::defaultPort)
{}

// FILE SYS

FileSys::FileSys() {
	// check if all (more or less) necessary files and directories exist
	if (setWorkingDir())
		std::cerr << "failed to set working directory" << std::endl;
	if (fileType(dirObjs) != FTYPE_DIR)
		std::cerr << "failed to find object directory" << std::endl;
	if (fileType(dirTexs) != FTYPE_DIR)
		std::cerr << "failed to find texture directory" << std::endl;
}

Settings* FileSys::loadSettings() {
	Settings* sets = new Settings();
	for (const string& line : readTextFile(fileSettings)) {
		if (pairStr il = readIniLine(line); il.first == iniKeywordMaximized)
			sets->maximized = stob(il.second);
		else if (il.first == iniKeywordScreen)
			sets->screen = valToEnum<Settings::Screen>(Settings::screenNames, il.second);
		else if (il.first == iniKeywordSize)
			sets->size = vec2i::get(il.second, strtoul, 0);
		else if (il.first == iniKeywordMode)
			sets->mode = strToDisp(il.second);
		else if (il.first == iniKeywordVsync)
			sets->vsync = Settings::VSync(valToEnum<int8>(Settings::vsyncNames, il.second) - 1);
		else if (il.first == iniKeywordSmooth)
			sets->smooth = valToEnum<Settings::Smooth>(Settings::smoothNames, il.second);
		else if (il.first == iniKeywordSamples)
			sets->samples = uint8(sstoul(il.second));
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
	text += makeIniLine(iniKeywordScreen, Settings::screenNames[uint8(sets->screen)]);
	text += makeIniLine(iniKeywordSize, sets->size.toString());
	text += makeIniLine(iniKeywordMode, dispToStr(sets->mode));
	text += makeIniLine(iniKeywordVsync, Settings::vsyncNames[int8(sets->vsync)+1]);
	text += makeIniLine(iniKeywordSamples, to_string(sets->samples));
	text += makeIniLine(iniKeywordSmooth, Settings::smoothNames[uint8(sets->smooth)]);
	text += makeIniLine(iniKeywordAddress, sets->address);
	text += makeIniLine(iniKeywordPort, to_string(sets->port));
	return writeTextFile(fileSettings, text);
}

Blueprint FileSys::loadObj(const string& file) {
	Blueprint obj;
	vector<string> lines = readTextFile(file);
	if (lines.empty())
		return obj;

	uint8 fill = 0;
	array<ushort, Vertex::size> begins = { 0, 0, 0 };
	for (const string& line : lines) {
		if (toupper(line[0]) == 'V') {
			if (toupper(line[1]) == 'T')
				obj.tuvs.emplace_back(stov<2>(&line[2]));
			else if (toupper(line[1]) == 'N')
				obj.norms.emplace_back(stov<3>(&line[2]));
			else
				obj.verts.emplace_back(stov<3>(&line[1]));
		} else if (toupper(line[0]) == 'F')
			fill |= readFace(&line[1], obj, begins);
		else if (int c = toupper(line[0]); c == 'O' || c == 'G')
			begins = { ushort(obj.verts.size()), ushort(obj.tuvs.size()), ushort(obj.norms.size()) };	// TODO: this might be a wrong assumption
	}
	if (fill & 1)
		obj.verts.emplace_back(0.f);
	if (fill & 2)
		obj.tuvs.emplace_back(0.f);
	if (fill & 4)
		obj.norms.emplace_back(0.f);
	return obj;
}

uint8 FileSys::readFace(const char* str, Blueprint& obj, const array<ushort, Vertex::size>& begins) {
	array<ushort, Vertex::size> sizes = {
		ushort(obj.verts.size()),
		ushort(obj.tuvs.size()),
		ushort(obj.norms.size())
	};
	array<Vertex, 3> face;
	uint8 v = 0, e = 0;
	for (char* end; *str && v < face.size();) {
		if (int n = int(strtol(str, &end, 0)); end != str) {
			face[v][e] = resolveObjId(n, sizes[e] - begins[e]) + begins[e];
			str = end;
			e++;
		} else if (*str == '/') {
			face[v][e] = sizes[e];
			e++;
		}
		if (e && (*str != '/' || e >= Vertex::size)) {
			e = 0;
			v++;
		}
		if (*str)
			str++;
	}
	if (v < face.size())
		return 0;
	
	std::swap(face[1], face[2]);	// model is CW, need CCW
	obj.elems.insert(obj.elems.end(), face.begin(), face.end());

	uint8 fill = 0;
	for (const Vertex& it : face)
		for (uint8 i = 0; i < 3; i++)
			if (it[i] == sizes[i])
				fill |= 1 << i;
	return fill;
}

ushort FileSys::resolveObjId(int id, ushort size) {
	if (ushort pid = ushort(id - 1); id > 0 && pid < size)
		return pid;
	if (ushort eid = size + ushort(id); id < 0 && eid < size)
		return eid;
	return size;
}

vector<string> FileSys::listDir(const string& drc, FileType filter) {
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW(stow(appDsep(drc) + "*").c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (!isDotName(data.cFileName) && atrcmp(data.dwFileAttributes, filter))
				entries.emplace_back(wtos(data.cFileName));
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
#else
	if (DIR* directory = opendir(drc.c_str())) {
		while (dirent* entry = readdir(directory))
			if (!isDotName(entry->d_name) && dtycmp(drc, entry, filter, true))
				entries.emplace_back(entry->d_name);
		closedir(directory);
	}
#endif
	std::sort(entries.begin(), entries.end());
	return entries;
}

int FileSys::setWorkingDir() {
	char* path = SDL_GetBasePath();
	if (!path)
		return 1;
#ifdef _WIN32
	int err = _wchdir(stow(path).c_str());
#else
	int err = chdir(path);
#endif
	SDL_free(path);
	return err;
}
#ifdef _WIN32
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
