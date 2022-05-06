#include "tests.h"
#include "prog/types.h"
#include "server/server.h"

static void testStringViewCat() {
	string str = "ref";
	assertEqual(""s + ""sv, "");
	assertEqual("a"s + "b"sv, "ab");
	assertEqual("ab"sv + "cd"s, "abcd");
	assertEqual(str + "vw"sv, "refvw");
	assertEqual("str"sv + str, "strref");
}

static void testReadWordM() {
	const char* str = "the quick\tbrown  fox\r\njumps\rover\v\vthe \tlazy\n dog";
	const char* words[] = { "the", "quick", "brown", "fox", "jumps", "over", "the", "lazy", "dog" };
	for (const char** exp = words; *str; ++exp)
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
	for (const char** exp = sents; *str; ++exp)
		assertEqual(strUnenclose(str), *exp);
}

static void testU8clen() {
	assertEqual(u8clen(0), 1);
	assertEqual(u8clen(0x7F), 1);
	assertEqual(u8clen(0x80), 0);
	assertEqual(u8clen(0xBF), 0);
	assertEqual(u8clen(0xC0), 2);
	assertEqual(u8clen(0xDF), 2);
	assertEqual(u8clen(0xE0), 3);
	assertEqual(u8clen(0xEF), 3);
	assertEqual(u8clen(0xF0), 4);
	assertEqual(u8clen(0xF7), 4);
	assertEqual(u8clen(0xF8), 0);
	assertEqual(u8clen(0xFF), 0);
}

static void testReadTextLines() {
	assertRange(readTextLines(""), vector<string>());
	assertRange(readTextLines("\r\n"), vector<string>());
	assertRange(readTextLines(" "), vector<string>({ " " }));
	assertRange(readTextLines("the quick\n\nbrown\n\rfox\r\njumps\tover\rthe\v lazy dog"), vector<string>({ "the quick", "brown", "fox", "jumps\tover", "the\v lazy dog" }));
}

static void testStrnatcmp() {
	assertEqual(strnatcmp("home", "home"), 0);
	assertGreater(strnatcmp("home", "Home"), 0);
	assertLess(strnatcmp("ahome", "Bhome"), 0);
	assertGreater(strnatcmp("home1", "Home10"), 0);
	assertLess(strnatcmp("Home1", "homeA"), 0);
	assertGreater(strnatcmp("home1b", "Home10a"), 0);
	assertLess(strnatcmp("Home1b", "homeAa"), 0);
}

static void testAppDsep() {
	assertEqual(appDsep(""), "/");
	assertEqual(appDsep("/home"), "/home/");
	assertEqual(appDsep("/home/"), "/home/");
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
	assertFalse(isSpace(0x80));
	assertFalse(isSpace(0xFF));
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
	assertTrue(notSpace(0x80));
	assertTrue(notSpace(0xFF));
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
	assertEqual(filename("home//file//"), "file");
}

static void testStrciEndsWith() {
	assertTrue(strciEndsWith("file.ext", ".ext"));
	assertTrue(strciEndsWith("file.ext", ".EXT"));
	assertTrue(strciEndsWith("file.EXT", ".ext"));
	assertFalse(strciEndsWith("file.png", ".ext"));
	assertFalse(strciEndsWith("file.ext", ".png"));
	assertFalse(strciEndsWith("ext", ".png"));
}

static void testParentPath() {
	assertEqual(parentPath(""), "");
	assertEqual(parentPath("/"), "/");
	assertEqual(parentPath("//"), "/");
	assertEqual(parentPath("/home"), "/");
	assertEqual(parentPath("//home"), "//");
	assertEqual(parentPath("/home/"), "/");
	assertEqual(parentPath("/home//"), "/");
	assertEqual(parentPath("home/file"), "home/");
	assertEqual(parentPath("home//file"), "home//");
	assertEqual(parentPath("home/file/"), "home/");
	assertEqual(parentPath("home/file//"), "home/");
	assertEqual(parentPath("home//file//"), "home//");
}

static void testStrToDisp() {
	assertEqual(strToDisp("  2\n1 "), SDL_DisplayMode({ 0, 2, 1, 0, nullptr }));
	assertEqual(strToDisp("\r\n1920 1080 \t 60\r"), SDL_DisplayMode({ 0, 1920, 1080, 60, nullptr }));
	assertEqual(strToDisp("128\t64  25\n0"), SDL_DisplayMode({ 0, 128, 64, 25, nullptr }));
}

static void testToStr() {
	assertEqual(toStr(true), "true"s);
	assertEqual(toStr(false), "false"s);
	assertEqual(toStr(0), "0");
	assertEqual(toStr(929u), "929");
	assertEqual(toStr(-54), "-54");
	assertEqual(toStr(0.f), "0");
	assertEqual(toStr(0.007f), "0.007");
	assertEqual(toStr(2.45f), "2.45");
	assertEqual(toStr(-86.125f), "-86.125");
	assertEqual(toStr(3.900f), "3.9");
	assertEqual(toStr(80.f), "80");
	assertEqual(toStr(0, 3), "000");
	assertEqual(toStr(2, 3), "002");
	assertEqual(toStr(40, 3), "040");
	assertEqual(toStr(100, 3), "100");
	assertEqual(toStr<2>(5u), "101");
	assertEqual(toStr<2>(36u), "100100");
	assertEqual(toStr<10>(0u), "0");
	assertEqual(toStr<10>(12u), "12");
	assertEqual(toStr<16>(0xA7u), "A7");
	assertEqual(toStr<16>(0x1Cu), "1C");
	assertEqual(toStr(glm::ivec3(1, 0, -1), " | "), "1 | 0 | -1");
	assertEqual(toStr(glm::vec3(4.2f, 0.f, -2.53f), "x"), "4.2x0x-2.53");
}

static void testToBool() {
	assertTrue(toBool("true"));
	assertTrue(toBool("TRUES"));
	assertFalse(toBool("false"));
	assertTrue(toBool("YES"));
	assertTrue(toBool("ya"));
	assertFalse(toBool("non"));
	assertTrue(toBool("ON"));
	assertTrue(toBool("on."));
	assertFalse(toBool("OFF"));
	assertTrue(toBool("1"));
	assertTrue(toBool("02"));
	assertFalse(toBool("0"));
	assertFalse(toBool("00"));
}

static void testStrToEnum() {
	assertEqual(strToEnum<uint8>(tileNames, "plains"), uint8(TileType::plains));
	assertEqual(strToEnum<uint8>(tileNames, "Plains"), uint8(TileType::plains));
	assertEqual(strToEnum<uint8>(tileNames, "forest"), uint8(TileType::forest));
	assertEqual(strToEnum<uint8>(tileNames, "FOREST"), uint8(TileType::forest));
	assertEqual(strToEnum<uint8>(tileNames, ""), uint8(TileType::empty));
	assertEqual(strToEnum<uint8>(tileNames, "garbage"), tileNames.size());
	assertEqual(strToEnum<uint8>(tileNames, "garbage", UINT8_MAX), UINT8_MAX);
}

static void testToNum() {
	assertEqual(toNum<int>("128"), 128);
	assertEqual(toNum<int>("-2"), -2);
	assertEqual(toNum<int>(" \r\n\t3"), 3);
	assertEqual(toNum<int>("+4"), 0);
	assertEqual(toNum<int>("abc"), 0);
	assertEqual(toNum<uint>("-1"), 0);
	assertEqual(toNum<uint8>("160"), 0);
	assertEqual(toNum<float>("128"), 128.f);
	assertEqual(toNum<float>("10.6"), 10.6f);
	assertEqual(toNum<float>("-2"), -2.f);
	assertEqual(toNum<float>("-0.5"), -0.5f);
	assertEqual(toNum<float>(" \r\n\t3"), 3.f);
	assertEqual(toNum<float>(" \r\n\t-4.1"), -4.1f);
	assertEqual(toNum<float>("+4"), 0);
	assertEqual(toNum<float>("abc"), 0);
}

static void testReadNumber() {
	const char* istr = "12 -1\t0\r\n-54";
	int ints[] = { 12, -1, 0, -54 };
	for (int* intp = ints; *istr; ++intp)
		assertEqual(readNumber<int>(istr), *intp);

	const char* fstr = "24.000 -2\t0.25\r\n-54.120 \t 0";
	float flts[] = { 24.f, -2.f, 0.25f, -54.12f, 0.f };
	for (float* fltp = flts; *fstr; ++fltp)
		assertEqual(readNumber<float>(fstr), *fltp);
}

static void testToVec() {
	assertEqual(toVec<glm::ivec4>(" 0 \n 3 "), glm::ivec4(0, 3, 0, 0));
	assertEqual(toVec<glm::ivec4>("\r\n1\t\t-1 24  -24 "), glm::ivec4(1, -1, 24, -24));
	assertEqual(toVec<glm::vec4>(" 0.01 \n -3.0 "), glm::vec4(0.01f, -3.f, 0.f, 0.f));
	assertEqual(toVec<glm::vec4>("\r\n1.0\t\t-1.50 24.99  -24 "), glm::vec4(1.f, -1.5f, 24.99f, -24.f));
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

static void testArguments() {
	const char* argv[] = { "arg", "v0", "-f", "-g", "v1", "-o", "s0", "v2", "-p" };
	Arguments arg(9, argv, { 'f', 'g' }, { 'o', 'p' });
	assertRange(arg.getVals(), vector<string>({ "v0", "v1", "v2" }));
	assertTrue(arg.hasFlag('f'));
	assertTrue(arg.hasFlag('g'));
	assertEqual(arg.getOpt('o'), "s0"s);
	assertEqual(arg.getOpt('p'), nullptr);
}

void testText() {
	puts("Running Text tests...");
	testStringViewCat();
	testReadWordM();
	testStrEnclose();
	testStrUnenclose();
	testU8clen();
	testReadTextLines();
	testStrnatcmp();
	testAppDsep();
	testIsSpace();
	testNotSpace();
	testFirstUpper();
	testTrim();
	testDelExt();
	testFilename();
	testStrciEndsWith();
	testParentPath();
	testStrToDisp();
	testToStr();
	testStrToEnum();
	testToBool();
	testToNum();
	testReadNumber();
	testToVec();
	testNumDigits();
	testArguments();
}
