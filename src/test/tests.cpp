#include "tests.h"

int testResult = EXIT_SUCCESS;

int main() {
	testServer();
	testText();
	testUtils();
	return testResult;
}
