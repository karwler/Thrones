#include "text.h"

bool hasExt(const string& path, const string& ext) {
	if (ext.empty())
		return true;

	string::const_reverse_iterator it = std::find_if(path.rbegin(), path.rend(), [](char c) -> bool { return c == '.' || isDsep(c); });
	return it != path.rend() && *it == '.' ? !strcicmp(string(it.base(), path.end()), ext) : false;
}

string filename(const string& path) {
	if (path[0] == '\0' || (isDsep(path[0]) && path[1] == '\0'))
		return "";

	string::const_iterator end = isDsep(path.back()) ? path.end() - 1 : path.end();
	return string(std::find_if(std::make_reverse_iterator(end), path.rend(), isDsep).base(), end);
}

string readWordM(const char*& pos) {
	const char* end;
	for (; isSpace(*pos); pos++);
	for (end = pos; notSpace(*end); end++);
	
	string str(pos, sizet(end - pos));
	pos = end;
	return str;
}
#ifdef _WIN32
string wtos(const wchar* src) {
	int len = Win::WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 1)
		return "";
	len--;
	
	string dst;
	dst.resize(len);
	Win::WideCharToMultiByte(CP_UTF8, 0, src, -1, dst.data(), len, nullptr, nullptr);
	return dst;
}

wstring stow(const string& src) {
	int len = Win::MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0);
	if (len <= 0)
		return L"";

	wstring dst;
	dst.resize(len);
	Win::MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), dst.data(), len);
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
	return li < ri && ri != string::npos ? trim(line.substr(li + 1, ri - li - 1)) : "";
}

pairStr readIniLine(const string& line) {
	sizet id = line.find_first_of('=');
	return id != string::npos ? pair(trim(line.substr(0, id)), trim(line.substr(id + 1))) : pairStr();
}
#ifdef _WIN32
void Arguments::setArgs(Win::PWSTR pCmdLine, const uset<string>& flg, const uset<string>& opt) {
	if (int argc; Win::LPWSTR* argv = Win::CommandLineToArgvW(pCmdLine, &argc)) {
		setArgs(argc, argv, wtos, flg, opt);
		Win::LocalFree(argv);
	}
}
#endif
