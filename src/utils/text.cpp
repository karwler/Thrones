#include "text.h"

string readWord(const char*& pos) {
	const char* end;
	for (; isSpace(*pos); pos++);
	for (end = pos; notSpace(*end); end++);
	
	string str(pos, sizet(end - pos));
	pos = end;
	return str;
}
#ifdef _WIN32
string wtos(const wchar* src) {
	int len = WideCharToMultiByte(CP_UTF8, 0, src, -1, nullptr, 0, nullptr, nullptr);
	if (len <= 1)
		return emptyStr;
	len--;
	
	string dst;
	dst.resize(len);
	WideCharToMultiByte(CP_UTF8, 0, src, -1, dst.data(), len, nullptr, nullptr);
	return dst;
}

wstring stow(const string& src) {
	int len = MultiByteToWideChar(CP_UTF8, 0, src.c_str(), int(src.length()), nullptr, 0);
	if (len <= 0)
		return L"";

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

vector<string> readTextFile(const string& file) {
	FILE* ifh = fopen(file.c_str(), "rb");
	if (!ifh)
		return {};
	
	fseek(ifh, 0, SEEK_END);
	sizet len = sizet(ftell(ifh));
	fseek(ifh, 0, SEEK_SET);

	string text;
	text.resize(len);
	fread(text.data(), sizeof(char), text.length(), ifh);
	fclose(ifh);

	vector<string> lines(1);
	for (char c : text) {
		if (c != '\n' && c != '\r')
			lines.back() += c;
		else if (!lines.back().empty())
			lines.push_back(emptyStr);
	}
	if (lines.back().empty())
		lines.pop_back();
	return lines;
}

bool writeTextFile(const string& file, const string& text) {
	if (FILE* ofh = fopen(file.c_str(), "wb")) {
		fputs(text.c_str(), ofh);
		return !fclose(ofh);
	}
	return false;
}

string readIniTitle(const string& line) {
	sizet li = line.find_first_of('[');
	sizet ri = line.find_last_of(']');
	return li < ri && ri != string::npos ? trim(line.substr(li + 1, ri - li - 1)) : emptyStr;
}

pairStr readIniLine(const string& line) {
	sizet id = line.find_first_of('=');
	return id != string::npos ? pair(trim(line.substr(0, id)), trim(line.substr(id + 1))) : pairStr();
}
#ifdef _WIN32
void Arguments::setArgs(PWSTR pCmdLine) {
	if (int argc; LPWSTR* argv = CommandLineToArgvW(pCmdLine, &argc)) {
		setArgs(argc, argv, wtos);
		LocalFree(argv);
	}
}
#endif
