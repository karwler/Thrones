#include "tests.h"
#include "engine/fileSys.h"

static void testReadIniLine() {
	assertEqual(IniLine(""), IniLine(IniLine::empty));
	assertEqual(IniLine("nut"), IniLine(IniLine::empty));
	assertEqual(IniLine("n]u[t"), IniLine(IniLine::empty));
	assertEqual(IniLine("[tit]"), IniLine(IniLine::title, "tit"));
	assertEqual(IniLine(" [ tit ] "), IniLine(IniLine::title, " tit "));
	assertEqual(IniLine("[t=t]"), IniLine(IniLine::prpVal, "[t", "", "t]"));
	assertEqual(IniLine("prop="), IniLine(IniLine::prpVal, "prop"));
	assertEqual(IniLine("prop = "), IniLine(IniLine::prpVal, "prop", "", " "));
	assertEqual(IniLine("=val"), IniLine(IniLine::prpVal, "", "", "val"));
	assertEqual(IniLine(" = val"), IniLine(IniLine::prpVal, "", "", " val"));
	assertEqual(IniLine("prop=val"), IniLine(IniLine::prpVal, "prop", "", "val"));
	assertEqual(IniLine("prop = val"), IniLine(IniLine::prpVal, "prop", "", " val"));
	assertEqual(IniLine("prop=val rot"), IniLine(IniLine::prpVal, "prop", "", "val rot"));
	assertEqual(IniLine("prop[]=val"), IniLine(IniLine::prpKeyVal, "prop", "", "val"));
	assertEqual(IniLine("prop[ ]=val"), IniLine(IniLine::prpKeyVal, "prop", "", "val"));
	assertEqual(IniLine("prop[key]="), IniLine(IniLine::prpKeyVal, "prop", "key"));
	assertEqual(IniLine("prop [ key ] = "), IniLine(IniLine::prpKeyVal, "prop", "key", " "));
	assertEqual(IniLine("[key]=val"), IniLine(IniLine::prpKeyVal, "", "key", "val"));
	assertEqual(IniLine(" [ key ] = val"), IniLine(IniLine::prpKeyVal, "", "key", " val"));
	assertEqual(IniLine("prop[key]=val"), IniLine(IniLine::prpKeyVal, "prop", "key", "val"));
	assertEqual(IniLine("prop [ key ] = val"), IniLine(IniLine::prpKeyVal, "prop", "key", " val"));
	assertEqual(IniLine("prop[key yak]=val rot"), IniLine(IniLine::prpKeyVal, "prop", "key yak", "val rot"));
}

void testFileSys() {
	puts("Running FileSys tests...");
	testReadIniLine();
}
