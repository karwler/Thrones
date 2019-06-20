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
	gamma(1.f),
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
#ifdef DEBUG
	if (Date date = Date::now(); ofLog = fopen(string("log_" + date.toString('-', '_', '-') + ".txt").c_str(), "wb"))
		writeLog(date.toString() + linend);
	else
		std::cerr << "failed to open log stream" << std::endl;
#endif
}
#ifdef DEBUG
FileSys::~FileSys() {
	if (ofLog)
		fclose(ofLog);
}

void FileSys::writeLog(const string& str) {
	if (ofLog) {
		bool ok = fwrite(str.c_str(), sizeof(*str.c_str()), str.length(), ofLog) == str.length();
		if (ok)
			ok = fwrite(linend, sizeof(*linend), sizeof(linend) / sizeof(*linend) - 1, ofLog) == sizeof(linend) / sizeof(*linend) - 1;
		if (!ok)
			std::cerr << "error during logging" << std::endl;
	}
}
#endif
Settings* FileSys::loadSettings() {
	Settings* sets = new Settings();
	for (const string& line : readTextFile(fileSettings)) {
		if (pairStr il = readIniLine(line); il.first == iniKeywordMaximized)
			sets->maximized = stob(il.second);
		else if (il.first == iniKeywordScreen)
			sets->screen = strToEnum<Settings::Screen>(Settings::screenNames, il.second);
		else if (il.first == iniKeywordSize)
			sets->size = vec2i::get(il.second, strtoul, 0);
		else if (il.first == iniKeywordMode)
			sets->mode = strToDisp(il.second);
		else if (il.first == iniKeywordVsync)
			sets->vsync = Settings::VSync(strToEnum<int8>(Settings::vsyncNames, il.second) - 1);
		else if (il.first == iniKeywordSmooth)
			sets->smooth = strToEnum<Settings::Smooth>(Settings::smoothNames, il.second);
		else if (il.first == iniKeywordSamples)
			sets->samples = uint8(sstoul(il.second));
		else if (il.first == iniKeywordGamma)
			sets->gamma = clampHigh(sstof(il.second), Settings::gammaMax);
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
	text += makeIniLine(iniKeywordVsync, Settings::vsyncNames[uint8(int8(sets->vsync)+1)]);
	text += makeIniLine(iniKeywordSamples, to_string(sets->samples));
	text += makeIniLine(iniKeywordSmooth, Settings::smoothNames[uint8(sets->smooth)]);
	text += makeIniLine(iniKeywordGamma, trimZero(to_string(sets->gamma)));
	text += makeIniLine(iniKeywordAddress, sets->address);
	text += makeIniLine(iniKeywordPort, to_string(sets->port));
	return writeTextFile(fileSettings, text);
}

vector<pair<string, Material>> FileSys::loadMtl(const string& file) {
	vector<pair<string, Material>> mtl(1);
	vector<string> lines = readTextFile(file);
	if (lines.empty())
		return mtl;
	
	sizet newLen = strlen(mtlKeywordNewmtl);
	for (const string& line : lines) {
		if (!strncicmp(line, mtlKeywordNewmtl, newLen)) {
			if (pair<string, Material> next(trim(line.substr(newLen)), Material()); mtl.back().first.empty())
				mtl.back() = std::move(next);
			else
				mtl.push_back(std::move(next));
		} else if (char c0 = char(toupper(line[0])); c0 == 'K') {
			if (char c1 = char(toupper(line[1])); c1 == 'A')
				mtl.back().second.ambient = stov<4>(&line[2], 1.f);
			else if (c1 == 'D')
				mtl.back().second.diffuse = stov<4>(&line[2], 1.f);
			else if (c1 == 'S')
				mtl.back().second.specular = stov<4>(&line[2], 1.f);
			else if (c1 == 'E')
				mtl.back().second.emission = stov<4>(&line[2], 1.f);
			else if (c1 == 'R')
				mtl.back().second.ambient.a = strtof(&line[1], nullptr);	// perhaps reverse? (Tr = 1 - d)
		} else if (c0 == 'N') {
			if (toupper(line[1]) == 'S')
				mtl.back().second.shine = strtof(&line[2], nullptr);
		} else if (c0 == 'D')
			mtl.back().second.ambient.a = strtof(&line[1], nullptr);
	}
	if (mtl.back().first.empty())
		mtl.pop_back();
	return mtl;
}

vector<pair<string, Blueprint>> FileSys::loadObj(const string& file) {
	vector<pair<string, Blueprint>> obj(1);
	vector<string> lines = readTextFile(file);
	if (lines.empty())
		return obj;

	uint8 fill = 0;
	array<ushort, Vertex::size> begins = { 0, 0, 0 };
	for (const string& line : lines) {
		if (char c0 = char(toupper(line[0])); c0 == 'V') {
			if (char c1 = char(toupper(line[1])); c1 == 'T')
				obj.back().second.tuvs.push_back(stov<2>(&line[2]));
			else if (c1 == 'N')
				obj.back().second.norms.push_back(stov<3>(&line[2]));
			else
				obj.back().second.verts.push_back(stov<3>(&line[1]));
		} else if (c0 == 'F')
			fill |= readFace(&line[1], obj.back().second, begins);
		else if (c0 == 'O') {
			begins = { ushort(begins[0] + obj.back().second.verts.size()), ushort(begins[1] + obj.back().second.tuvs.size()), ushort(begins[2] + obj.back().second.norms.size()) };
			fillUpObj(fill, obj.back().second);
			if (pair<string, Blueprint> next(trim(line.substr(1)), Blueprint()); obj.back().first.empty() || obj.back().second.empty())
				obj.back() = std::move(next);
			else
				obj.push_back(std::move(next));
		}
	}
	fillUpObj(fill, obj.back().second);
	if (obj.back().first.empty() || obj.back().second.empty())
		obj.pop_back();
	return  obj;
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
			face[v][e] = resolveObjId(n, sizes[e] + begins[e]) - begins[e];
			str = end;
			e++;
		} else if (*str == '/') {
			face[v][e] = sizes[e];
			e++;
		}
		if (e && (*str != '/' || e >= Vertex::size)) {
			std::copy(sizes.begin() + e, sizes.end(), face[v].begin() + e);
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

void FileSys::fillUpObj(uint8& fill, Blueprint& obj) {
	if (fill & 1)
		obj.verts.emplace_back(0.f);
	if (fill & 2)
		obj.tuvs.emplace_back(0.f);
	if (fill & 4)
		obj.norms.emplace_back(0.f);
	fill = 0;
}

vector<string> FileSys::listDir(const string& drc, FileType filter, const string& ext) {
	vector<string> entries;
#ifdef _WIN32
	WIN32_FIND_DATAW data;
	if (HANDLE hFind = FindFirstFileW(stow(appDsep(drc) + "*").c_str(), &data); hFind != INVALID_HANDLE_VALUE) {
		do {
			if (string fname = wtos(data.cFileName); !isDotName(fname) && atrcmp(data.dwFileAttributes, filter) && hasExt(fname, ext))
				entries.push_back(std::move(fname));
		} while (FindNextFileW(hFind, &data));
		FindClose(hFind);
	}
#else
	if (DIR* directory = opendir(drc.c_str())) {
		while (dirent* entry = readdir(directory))
			if (string fname = entry->d_name; !isDotName(fname) && dtycmp(drc, entry, filter, true) && hasExt(fname, ext))
				entries.push_back(std::move(fname));
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
