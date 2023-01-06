#include "text.h"
#include <ctime>
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

string_view readWord(const char*& pos) {
	sizet i = 0;
	for (; isSpace(*pos); ++pos);
	for (; notSpace(pos[i]); ++i);

	string_view str(pos, i);
	pos += i;
	return str;
}

string strEnclose(string_view str) {
	string txt(str);
	for (sizet i = txt.find_first_of("\"\\"); i < txt.length(); i = txt.find_first_of("\"\\", i + 2))
		txt.insert(txt.begin() + i, '\\');
	return '"' + txt + '"';
}

string strUnenclose(const char*& str) {
	const char* pos = strchr(str, '"');
	if (!pos)
		return str += strlen(str);
	++pos;

	string quote;
	for (sizet len; *pos && *pos != '"'; pos += len) {
		len = strcspn(pos, "\"\\");
		quote.append(pos, len);
		if (pos[len] == '\\') {
			if (char ch = pos[len + 1]) {
				quote += ch;
				pos += 2;
			} else
				++pos;
		}
	}
	str = *pos ? pos + 1 : pos;
	return quote;
}

static int natCmpLetter(int a, int b) {
	if (a != b) {
		int au = toupper(a), bu = toupper(b);
		return au != bu ? au - bu : a - b;
	}
	return 0;
}

static int natCmpLeft(const char* a, const char* b) {
	for (;; ++a, ++b) {
		bool nad = !isdigit(*a), nbd = !isdigit(*b);
		if (nad && nbd)
			return 0;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (int dif = natCmpLetter(*a, *b))
			return dif;
	}
}

static int natCmpRight(const char* a, const char* b) {
	for (int bias = 0;; ++a, ++b) {
		bool nad = !isdigit(*a), nbd = !isdigit(*b);
		if (nad && nbd)
			return bias;
		if (nad)
			return -1;
		if (nbd)
			return 1;
		if (!(*a || *b))
			return bias;
		if (int dif = natCmpLetter(*a, *b); dif && !bias)
			bias = dif;
	}
}

int strnatcmp(const char* a, const char* b) {
	for (;; ++a, ++b) {
		char ca = *a, cb = *b;
		for (; isSpace(ca); ca = *++a);
		for (; isSpace(cb); cb = *++b);

		if (isdigit(ca) && isdigit(cb))
			if (int dif = ca == '0' || cb == '0' ? natCmpLeft(a, b) : natCmpRight(a, b))
				return dif;
		if (!(ca || cb))
			return 0;
		if (int dif = natCmpLetter(*a, *b))
			return dif;
	}
}

uint8 u8clen(char c) {
	if (!(c & 0x80))
		return 1;
	if ((c & 0xE0) == 0xC0)
		return 2;
	if ((c & 0xF0) == 0xE0)
		return 3;
	if ((c & 0xF8) == 0xF0)
		return 4;
	return 0;
}

vector<string> readTextLines(string_view text) {
	vector<string> lines;
	constexpr char nl[] = "\n\r";
	for (sizet e, p = text.find_first_not_of(nl); p < text.length(); p = text.find_first_not_of(nl, e))
		if (e = text.find_first_of(nl, p); sizet len = e - p)
			lines.emplace_back(text.data() + p, len);
	return lines;
}

bool createDirectories(const string& path) {
	bool ok = true;
#ifdef _WIN32
	wstring wpath = sstow(path);
	if (DWORD rc = GetFileAttributesW(wpath.c_str()); rc == INVALID_FILE_ATTRIBUTES || !(rc & FILE_ATTRIBUTE_DIRECTORY))
		for (wstring::iterator end = std::find_if_not(wpath.begin(), wpath.end(), isDsep); end != wpath.end(); end = std::find_if_not(end, wpath.end(), isDsep)) {
			end = std::find_if(end, wpath.end(), isDsep);
			wchar tmp = *end;
			*end = '\0';
			ok = CreateDirectoryW(wpath.c_str(), nullptr);
			*end = tmp;
		}
#else
	if (struct stat ps; stat(path.c_str(), &ps) || (ps.st_mode & S_IFMT) != S_IFDIR)
		for (string::const_iterator end = std::find_if_not(path.begin(), path.end(), isDsep); end != path.end(); end = std::find_if_not(end, path.end(), isDsep)) {
			end = std::find_if(end, path.end(), isDsep);
			ok = !mkdir(string(path.begin(), end).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
		}
#endif
	return ok;
}

SDL_DisplayMode strToDisp(string_view str) {
	int w = readNumber<uint>(str);
	int h = readNumber<uint>(str);
	int r = readNumber<uint>(str);
	uint32 f = readNumber<uint32>(str);
	return { f, w, h, r, nullptr };
}

#ifdef _WIN32
string swtos(wstring_view src) {
	string dst;
	if (int len = WideCharToMultiByte(CP_UTF8, 0, src.data(), src.length(), nullptr, 0, nullptr, nullptr); len > 0) {
		dst.resize(len);
		WideCharToMultiByte(CP_UTF8, 0, src.data(), src.length(), dst.data(), len, nullptr, nullptr);
	}
	return dst;
}

wstring sstow(string_view src) {
	wstring dst;
	if (int len = MultiByteToWideChar(CP_UTF8, 0, src.data(), src.length(), nullptr, 0); len > 0) {
		dst.resize(len);
		MultiByteToWideChar(CP_UTF8, 0, src.data(), src.length(), dst.data(), len);
	}
	return dst;
}

string lastErrorMessage() {
	wchar* buff = nullptr;
	string msg = FormatMessageW(FORMAT_MESSAGE_ALLOCATE_BUFFER | FORMAT_MESSAGE_FROM_SYSTEM, nullptr, GetLastError(), 0, reinterpret_cast<LPWSTR>(&buff), 0, nullptr) ? swtos(buff) : string();
	if (buff)
		LocalFree(buff);
	return string(trim(msg));
}
#endif

// ARGUMENTS

#ifdef _WIN32
void Arguments::setArgs(int argc, const wchar* const* argv, const uset<char>& flg, const uset<char>& opt) {
	setArgs(argc, argv, swtos, flg, opt);
}
#endif

void Arguments::setArgs(int argc, const char* const* argv, const uset<char>& flg, const uset<char>& opt) {
	setArgs(argc, argv, [](const char* str) -> string { return str; }, flg, opt);
}

template <class C, class F>
void Arguments::setArgs(int argc, const C* const* argv, F conv, const uset<char>& flg, const uset<char>& opt) {
	for (int i = 0; i < argc; ++i) {
		if (char key; argv[i][0] == '-') {
			for (int j = 1; (key = char(argv[i][j])); ++j) {
				if (!argv[i][j + 1] && i + 1 < argc && opt.count(key)) {
					opts.emplace(key, conv(argv[++i]));
					break;
				}
				if (flg.count(key))
					flags.insert(key);
			}
		} else
			vals.push_back(conv(argv[i] + (argv[i][0] == '\\' && (argv[i][1] == '-' || argv[i][1] == '\\'))));
	}
	vals.shrink_to_fit();
}

// DATE TIME

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
	tm tim;
#ifdef _WIN32
	localtime_s(&tim, &rawt);
#else
	localtime_r(&rawt, &tim);
#endif
	return DateTime(tim.tm_sec, tim.tm_min, tim.tm_hour, tim.tm_mday, tim.tm_mon + 1, tim.tm_year + 1900, tim.tm_wday ? tim.tm_wday : 7);
}
