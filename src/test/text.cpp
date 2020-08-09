#include "tests.h"
#include "server/server.h"

static void testReadMem() {
	uint8 mem[] = { 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08 };
	assertEqual(readMem<uint8>(mem), 0x10u);
	assertEqual(readMem<uint16>(mem), 0x2010u);
	assertEqual(readMem<uint32>(mem), 0x80402010u);
	assertEqual(readMem<uint64>(mem), 0x0804020180402010u);
}

static void testWriteMem() {
	uint8 mem[8], exp[] = { 0x10, 0x20, 0x40, 0x80, 0x1, 0x2, 0x4, 0x8 };
	writeMem(mem, uint8(0x10));
	assertMemory(mem, exp, 1);
	writeMem(mem, uint16(0x2010));
	assertMemory(mem, exp, 2);
	writeMem(mem, uint32(0x80402010));
	assertMemory(mem, exp, 4);
	writeMem(mem, uint64(0x0804020180402010));
	assertMemory(mem, exp, 8);
}

static void testFilename() {
	assertEqual(filename(""), "");
	assertEqual(filename("/"), "");
	assertEqual(filename("//"), "");
	assertEqual(filename("/home"), "home");
	assertEqual(filename("//home"), "home");
	assertEqual(filename("/home/"), "home");
	assertEqual(filename("/home//"), "home");
	assertEqual(filename("home/file"), "file");
	assertEqual(filename("home//file"), "file");
	assertEqual(filename("home/file/"), "file");
	assertEqual(filename("home/file//"), "file");
}

static void testReadWordM() {
	const char* str = "the quick\tbrown  fox\r\njumps\rover\v\vthe \tlazy\n dog";
	const char* words[] = { "the", "quick", "brown", "fox", "jumps", "over", "the", "lazy", "dog" };
	for (const char** exp = words; *str; exp++)
		assertEqual(readWord(str), *exp);
}

static void testStrEnclose() {
	assertEqual(strEnclose(""), R"r("")r");
	assertEqual(strEnclose(R"r(\"\)r"), R"r("\\\"\\")r");
	assertEqual(strEnclose(R"r("\")r"), R"r("\"\\\"")r");
	assertEqual(strEnclose(R"r(the "quick" \brown \"fox jumps")r"), R"r("the \"quick\" \\brown \\\"fox jumps\"")r");
}

static void testStrUnenclose() {
	const char* str = R"r( "the quick"  "\"brown\" fox\\"	"jumps \"over\" the""\"lazy  dog\""	)r";
	const char* sents[] = { "the quick", "\"brown\" fox\\", "jumps \"over\" the", "\"lazy  dog\"", "" };
	for (const char** exp = sents; *str; exp++)
		assertEqual(strUnenclose(str), *exp);
}

static void testU8clen() {
	assertEqual(u8clen(0), 1);
	assertEqual(u8clen(0x7F), 1);
	assertEqual(u8clen(char(0x80)), 0);
	assertEqual(u8clen(char(0xBF)), 0);
	assertEqual(u8clen(char(0xC0)), 2);
	assertEqual(u8clen(char(0xDF)), 2);
	assertEqual(u8clen(char(0xE0)), 3);
	assertEqual(u8clen(char(0xEF)), 3);
	assertEqual(u8clen(char(0xF0)), 4);
	assertEqual(u8clen(char(0xF7)), 4);
	assertEqual(u8clen(char(0xF8)), 0);
	assertEqual(u8clen(char(0xFF)), 0);
}

static void testReadTextLines() {
	assertRange(readTextLines(""), vector<string>());
	assertRange(readTextLines("\r\n"), vector<string>());
	assertRange(readTextLines(" "), vector<string>({ " " }));
	assertRange(readTextLines("the quick\n\nbrown\n\rfox\r\njumps\tover\rthe\v lazy dog"), vector<string>({ "the quick", "brown", "fox", "jumps\tover", "the\v lazy dog" }));
}

static void testReadIniTitle() {
	assertEqual(readIniTitle(""), "");
	assertEqual(readIniTitle("no=tit"), "");
	assertEqual(readIniTitle("]notit["), "");
	assertEqual(readIniTitle("n]ot[it"), "");
	assertEqual(readIniTitle("[tit]"), "tit");
	assertEqual(readIniTitle(" [tit]\t"), "tit");
	assertEqual(readIniTitle("\r\n[tit]  "), "tit");
}

static void testReadIniLine() {
	assertEqual(readIniLine(""), pair("", ""));
	assertEqual(readIniLine("nuttin"), pair("", ""));
	assertEqual(readIniLine("[tit]"), pair("", ""));
	assertEqual(readIniLine("key="), pair("key", ""));
	assertEqual(readIniLine("key = "), pair("key", ""));
	assertEqual(readIniLine("=val"), pair("", "val"));
	assertEqual(readIniLine(" = val"), pair("", "val"));
	assertEqual(readIniLine("key=val"), pair("key", "val"));
	assertEqual(readIniLine("key = val"), pair("key", "val"));
	assertEqual(readIniLine("key=val str"), pair("key", "val str"));
}

static void testStrcicmp() {
	assertEqual(strcicmp("bob", "bob"), 0);
	assertEqual(strcicmp("bob", "Bob"), 0);
	assertEqual(strcicmp("bobs", "Bob"), 's');
	assertEqual(strcicmp("bob", "Bobs"), -'s');
}

static void testStrncicmp() {
	assertEqual(strncicmp("a", "b", 0), 0);
	assertEqual(strncicmp("boa", "bob", 2), 0);
	assertEqual(strncicmp("boa", "Bob", 2), 0);
	assertEqual(strncicmp("boa", "Bob", 4), 'a' - 'b');
	assertEqual(strncicmp("bobs", "Bob", 4), 's');
	assertEqual(strncicmp("bob", "Bobs", 4), -'s');
}

static void testStrnatcmp() {
	assertEqual(strnatcmp("home", "home"), 0);
	assertEqual(strnatcmp("home", "Home"), 1);
	assertEqual(strnatcmp("ahome", "Bhome"), -1);
	assertEqual(strnatcmp("home1", "Home10"), 1);
	assertEqual(strnatcmp("Home1", "homeA"), -1);
	assertEqual(strnatcmp("home1b", "Home10a"), 1);
	assertEqual(strnatcmp("Home1b", "homeAa"), -1);
}

static void testIsSpace() {
	assertFalse(isSpace('\0'));
	assertTrue(isSpace('\a'));
	assertTrue(isSpace('\t'));
	assertTrue(isSpace('\n'));
	assertTrue(isSpace('\r'));
	assertTrue(isSpace(' '));
	assertFalse(isSpace('.'));
	assertFalse(isSpace('0'));
	assertFalse(isSpace('A'));
	assertFalse(isSpace('_'));
	assertFalse(isSpace('a'));
	assertTrue(isSpace(0x7F));
	assertFalse(isSpace(char(0x80)));
	assertFalse(isSpace(char(0xFF)));
}

static void testNotSpace() {
	assertFalse(notSpace('\0'));
	assertFalse(notSpace('\a'));
	assertFalse(notSpace('\t'));
	assertFalse(notSpace('\n'));
	assertFalse(notSpace('\r'));
	assertFalse(notSpace(' '));
	assertTrue(notSpace('.'));
	assertTrue(notSpace('0'));
	assertTrue(notSpace('A'));
	assertTrue(notSpace('_'));
	assertTrue(notSpace('a'));
	assertFalse(notSpace(0x7F));
	assertTrue(notSpace(char(0x80)));
	assertTrue(notSpace(char(0xFF)));
}

static void testFirstUpper() {
	assertEqual(firstUpper("abc"), "Abc");
	assertEqual(firstUpper("Abc"), "Abc");
	assertEqual(firstUpper("ABC"), "ABC");
}

static void testTrim() {
	assertEqual(trim("txt"), "txt");
	assertEqual(trim(" txt  "), "txt");
	assertEqual(trim("\t\t t x t \n"), "t x t");
}

static void testDelExt() {
	assertEqual(delExt(""), "");
	assertEqual(delExt(".o"), ".o");
	assertEqual(delExt("a.o"), "a");
	assertEqual(delExt("a.tar.gz"), "a.tar");
	assertEqual(delExt("dir/.o"), "dir/.o");
	assertEqual(delExt("dir/a"), "dir/a");
	assertEqual(delExt("dir/a.o"), "dir/a");
	assertEqual(delExt("dir//a.o"), "dir//a");
	assertEqual(delExt("/dir/a.o"), "/dir/a");
}

static void testStrToDisp() {
	assertEqual(strToDisp("  2\n1 "), SDL_DisplayMode({ 0, 2, 1, 0, nullptr }));
	assertEqual(strToDisp("\r\n1920 1080 \t 60\r"), SDL_DisplayMode({ 0, 1920, 1080, 60, nullptr }));
	assertEqual(strToDisp("128\t64  25\n0"), SDL_DisplayMode({ 0, 128, 64, 25, nullptr }));
}

static void testToStr() {
	assertEqual(toStr(0), "0");
	assertEqual(toStr(929u), "929");
	assertEqual(toStr(-54), "-54");
	assertEqual(toStr(0.f), "0");
	assertEqual(toStr(0.007f), "0.007");
	assertEqual(toStr(2.45f), "2.45");
	assertEqual(toStr(-86.125f), "-86.125");
	assertEqual(toStr(0, 3), "000");
	assertEqual(toStr(2, 3), "002");
	assertEqual(toStr(40, 3), "040");
	assertEqual(toStr(100, 3), "100");
	assertEqual(toStr(glm::ivec3(1, 0, -1), " | "), "1 | 0 | -1");
	assertEqual(toStr(glm::vec3(4.2f, 0.f, -2.53f), "x"), "4.2x0x-2.53");
}

static void testStob() {
	assertTrue(stob("true"));
	assertTrue(stob("TRUES"));
	assertFalse(stob("false"));
	assertTrue(stob("YES"));
	assertTrue(stob("ya"));
	assertFalse(stob("non"));
	assertTrue(stob("ON"));
	assertTrue(stob("on."));
	assertFalse(stob("OFF"));
	assertTrue(stob("1"));
	assertTrue(stob("02"));
	assertFalse(stob("0"));
	assertFalse(stob("00"));
}

static void testBtos() {
	assertEqual(btos(true), string("true"));
	assertEqual(btos(false), string("false"));
}

static void testStrToEnum() {
	assertEqual(strToEnum<uint8>(Com::tileNames, "plains"), uint8(Com::Tile::plains));
	assertEqual(strToEnum<uint8>(Com::tileNames, "Plains"), uint8(Com::Tile::plains));
	assertEqual(strToEnum<uint8>(Com::tileNames, "forest"), uint8(Com::Tile::forest));
	assertEqual(strToEnum<uint8>(Com::tileNames, "FOREST"), uint8(Com::Tile::forest));
	assertEqual(strToEnum<uint8>(Com::tileNames, ""), uint8(Com::Tile::empty));
	assertEqual(strToEnum<uint8>(Com::tileNames, "garbage"), Com::tileNames.size());
	assertEqual(strToEnum<uint8>(Com::tileNames, "garbage", UINT8_MAX), UINT8_MAX);
}

static void testReadNumber() {
	const char* istr = "12 -1\t0\r\n-54";
	int ints[] = { 12, -1, 0, -54 };
	for (int* intp = ints; *istr; intp++)
		assertEqual(readNumber<int>(istr, strtol, 0), *intp);

	const char* fstr = "24.000 -2\t0.25\r\n-54.120 \t 0";
	float flts[] = { 24.f, -2.f, 0.25f, -54.12f, 0.f };
	for (float* fltp = flts; *fstr; fltp++)
		assertEqual(readNumber<float>(fstr, strtof), *fltp);
}

static void testStoxv() {
	assertEqual(stoiv<glm::ivec4>(" 0 \n 3 ", strtol), glm::ivec4(0, 3, 0, 0));
	assertEqual(stoiv<glm::ivec4>("\r\n1\t\t-1 24  -24 ", strtol), glm::ivec4(1, -1, 24, -24));
	assertEqual(stofv<glm::vec4>(" 0.01 \n -3.0 "), glm::vec4(0.01f, -3.f, 0.f, 0.f));
	assertEqual(stofv<glm::vec4>("\r\n1.0\t\t-1.50 24.99  -24 "), glm::vec4(1.f, -1.5f, 24.99f, -24.f));
}

static void testCycle() {
	assertEqual(cycle(0u, 1u, 1), 0u);
	assertEqual(cycle(0u, 1u, -1), 0u);
	assertEqual(cycle(0u, 1u, 2), 0u);
	assertEqual(cycle(0u, 1u, -2), 0u);
	assertEqual(cycle(1u, 2u, 4), 1u);
	assertEqual(cycle(1u, 2u, -4), 1u);
	assertEqual(cycle(1u, 3u, 1), 2u);
	assertEqual(cycle(1u, 3u, -1), 0u);
	assertEqual(cycle(2u, 3u, 2), 1u);
	assertEqual(cycle(2u, 3u, -2), 0u);
	assertEqual(cycle(1u, 3u, 3), 1u);
	assertEqual(cycle(1u, 3u, -3), 1u);
	assertEqual(cycle(2u, 3u, 4), 0u);
	assertEqual(cycle(2u, 3u, -4), 1u);
}

static void testNumDigits() {
	assertEqual(numDigits(00u, 8), 1);
	assertEqual(numDigits(01u, 8), 1);
	assertEqual(numDigits(07u, 8), 1);
	assertEqual(numDigits(010u, 8), 2);
	assertEqual(numDigits(011u, 8), 2);
	assertEqual(numDigits(077u, 8), 2);
	assertEqual(numDigits(0100u, 8), 3);
	assertEqual(numDigits(0101u, 8), 3);
	assertEqual(numDigits(0u, 10), 1);
	assertEqual(numDigits(1u, 10), 1);
	assertEqual(numDigits(9u, 10), 1);
	assertEqual(numDigits(10u, 10), 2);
	assertEqual(numDigits(11u, 10), 2);
	assertEqual(numDigits(99u, 10), 2);
	assertEqual(numDigits(100u, 10), 3);
	assertEqual(numDigits(101u, 10), 3);
	assertEqual(numDigits(0x0u, 16), 1);
	assertEqual(numDigits(0x1u, 16), 1);
	assertEqual(numDigits(0xFu, 16), 1);
	assertEqual(numDigits(0x10u, 16), 2);
	assertEqual(numDigits(0x11u, 16), 2);
	assertEqual(numDigits(0x99u, 16), 2);
	assertEqual(numDigits(0x100u, 16), 3);
	assertEqual(numDigits(0x101u, 16), 3);
}

static void testSortNames() {
	umap<string, int> vals = { pair("a01", 0), pair("zb", 0), pair("a1", 0), pair("Zb", 0), pair("a10", 0) };
	assertRange(sortNames(vals), vector<string>({ "a01", "a1", "a10", "Zb", "zb" }));
}

static void testArguments() {
	const char* argv[] = { "arg", "v0", "-f", "-g", "v1", "-o", "s0", "v2", "-p" };
	Arguments arg(9, argv, { 'f', 'g' }, { 'o', 'p' });
	assertRange(arg.getVals(), vector<string>({ "v0", "v1", "v2" }));
	assertTrue(arg.hasFlag('f'));
	assertTrue(arg.hasFlag('g'));
	assertEqual(arg.getOpt('o'), string("s0"));
	assertEqual(arg.getOpt('p'), nullptr);
}

void testText() {
	testReadMem();
	testWriteMem();
	testFilename();
	testReadWordM();
	testStrEnclose();
	testStrUnenclose();
	testU8clen();
	testReadTextLines();
	testReadIniTitle();
	testReadIniLine();
	testStrcicmp();
	testStrncicmp();
	testStrnatcmp();
	testIsSpace();
	testNotSpace();
	testFirstUpper();
	testTrim();
	testDelExt();
	testStrToDisp();
	testToStr();
	testStob();
	testBtos();
	testStrToEnum();
	testReadNumber();
	testStoxv();
	testCycle();
	testNumDigits();
	testSortNames();
	testArguments();
}
