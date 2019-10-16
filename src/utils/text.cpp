#include "text.h"

string filename(const string& path) {
	if (path[0] == '\0' || (isDsep(path[0]) && path[1] == '\0'))
		return string();

	string::const_iterator end = isDsep(path.back()) ? path.end() - 1 : path.end();
	return string(std::find_if(string::const_reverse_iterator(end), path.rend(), isDsep).base(), end);
}

string readWordM(const char*& pos) {
	const char* end;
	for (; isSpace(*pos); pos++);
	for (end = pos; notSpace(*end); end++);
	
	string str(pos, sizet(end - pos));
	pos = end;
	return str;
}

static inline int natCmpLetter(int a, int b) {
	if (a != b) {
		int au = toupper(a), bu = toupper(b);
		return au != bu ? au - bu : b - a;
	}
	return 0;
}

static inline int natCmpLeft(const char* a, const char* b) {
	for (;; a++, b++) {
		bool nad = notDigit(*a), nbd = notDigit(*b);
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

static inline int natCmpRight(const char* a, const char* b) {
	for (int bias = 0;; a++, b++) {
		bool nad = notDigit(*a), nbd = notDigit(*b);
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
	for (;; a++, b++) {
		char ca = *a, cb = *b;
		for (; isSpace(ca); ca = *++a);
		for (; isSpace(cb); cb = *++b);

		if (isDigit(ca) && isDigit(cb))
			if (int dif = ca == '0' || cb == '0' ? natCmpLeft(a, b) : natCmpRight(a, b))
				return dif;
		if (!(ca || cb))
			return 0;
		if (int dif = natCmpLetter(*a, *b))
			return dif;
	}
}

#ifdef _WIN32
string wtos(const wchar* src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 1)
		return string();
	len--;
	
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

SDL_DisplayMode strToDisp(const string& str) {
	const char* pos = str.c_str();
	int w = readNumber<int>(pos, strtoul, 0);
	int h = readNumber<int>(pos, strtoul, 0);
	int r = readNumber<int>(pos, strtoul, 0);
	uint32 f = readNumber<uint32>(pos, strtoul, 0);
	return { f, w, h, r, nullptr };
}

vector<string> readFileLines(const string& file) {
	vector<string> lines(1);
	for (char c : readFile(file)) {
		if (c != '\n' && c != '\r')
			lines.back() += c;
		else if (!lines.back().empty())
			lines.emplace_back();
	}
	if (lines.back().empty())
		lines.pop_back();
	return lines;
}

bool writeFile(const string& file, const string& text) {
	if (FILE* ofh = fopen(file.c_str(), defaultWriteMode)) {
		fwrite(text.c_str(), sizeof(*text.c_str()), text.length(), ofh);
		return !fclose(ofh);
	}
	return false;
}

string readIniTitle(const string& line) {
	sizet li = line.find_first_of('[');
	sizet ri = line.find_last_of(']');
	return li < ri && ri != string::npos ? trim(line.substr(li + 1, ri - li - 1)) : string();
}

pairStr readIniLine(const string& line) {
	sizet id = line.find_first_of('=');
	return id != string::npos ? pair(trim(line.substr(0, id)), trim(line.substr(id + 1))) : pairStr();
}

#ifdef _WIN32
void Arguments::setArgs(PWSTR pCmdLine, const uset<char>& flg, const uset<char>& opt) {
	if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
		setArgs(argc, argv, wtos, flg, opt);
		LocalFree(argv);
	}
}
#endif
