#include "text.h"
#ifdef _WIN32
#include <windows.h>
#else
#include <sys/stat.h>
#endif

string filename(const string& path) {
	if (path.empty() || std::all_of(path.begin(), path.end(), isDsep))
		return string();

	string::const_reverse_iterator end = std::find_if_not(path.rbegin(), path.rend(), isDsep);
	return string(std::find_if(end, path.rend(), isDsep).base(), end.base());
}

string readWord(const char*& pos) {
	const char* end;
	for (; isSpace(*pos); ++pos);
	for (end = pos; notSpace(*end); ++end);

	string str(pos, end);
	pos = end;
	return str;
}

string strEnclose(string str) {
	for (sizet i = str.find_first_of("\"\\"); i < str.length(); i = str.find_first_of("\"\\", i + 2))
		str.insert(str.begin() + i, '\\');
	return '"' + str + '"';
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
			if (char ch = pos[len+1]) {
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

vector<string> readTextLines(const string& text) {
	vector<string> lines;
	constexpr char nl[] = "\n\r";
	for (sizet e, p = text.find_first_not_of(nl); p < text.length(); p = text.find_first_not_of(nl, e))
		if (e = text.find_first_of(nl, p); sizet len = e - p)
			lines.push_back(text.substr(p, len));
	return lines;
}

void createDirectories(const string& path) {
	for (string::const_iterator end = std::find_if_not(path.begin(), path.end(), isDsep); end != path.end(); end = std::find_if_not(end, path.end(), isDsep)) {
		end = std::find_if(end, path.end(), isDsep);
#ifdef _WIN32
		CreateDirectoryW(stow(string(path.begin(), end)).c_str(), nullptr);
#else
		mkdir(string(path.begin(), end).c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
#endif
	}
}

SDL_DisplayMode strToDisp(const string& str) {
	const char* pos = str.c_str();
	int w = readNumber<int>(pos, strtoul, 0);
	int h = readNumber<int>(pos, strtoul, 0);
	int r = readNumber<int>(pos, strtoul, 0);
	uint32 f = readNumber<uint32>(pos, strtoul, 0);
	return { f, w, h, r, nullptr };
}

#ifdef _WIN32
string wtos(const wchar* src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 1)
		return string();
	--len;

	string dst;
	dst.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, dst.data(), len, nullptr, nullptr);
	return dst;
}

wstring stow(const string& src) {
	int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0);
	if (len <= 0)
		return wstring();

	wstring dst;
	dst.resize(len);
	MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), dst.data(), len);
	return dst;
}
#endif

// ARGUMENTS

#ifdef _WIN32
void Arguments::setArgs(int argc, const wchar* const* argv, const uset<char>& flg, const uset<char>& opt) {
	setArgs(argc - 1, argv + 1, wtos, flg, opt);
}
#endif

void Arguments::setArgs(int argc, const char* const* argv, const uset<char>& flg, const uset<char>& opt) {
	setArgs(argc - 1, argv + 1, [](const char* str) -> string { return str; }, flg, opt);
}

template <class C, class F>
void Arguments::setArgs(int argc, const C* const* argv, F conv, const uset<char>& flg, const uset<char>& opt) {
	for (int i = 0; i < argc; ++i) {
		if (char key; argv[i][0] == '-') {
			for (int j = 1; (key = char(argv[i][j])); ++j) {
				if (!argv[i][j+1] && i + 1 < argc && opt.count(key)) {
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
