#include "tests.h"

int testResult = EXIT_SUCCESS;

bool operator!=(const IniLine& a, const IniLine& b) {
	if (a.type != b.type)
		return true;
	switch (a.type) {
	case IniLine::prpVal:
		return a.prp != b.prp || a.val != b.val;
	case IniLine::prpKeyVal:
		return a.prp != b.prp || a.key != b.key || a.val != b.val;
	case IniLine::title:
		return a.prp != b.prp;
	}
	return false;
}

int main() {
	testAlias();
	testFileSys();
	testServer();
	testText();
	testUtils();
	return testResult;
}
