#include "server.h"

std::default_random_engine createRandomEngine() {
	std::default_random_engine randGen;
	try {
		std::random_device rd;
		randGen.seed(rd());
	} catch (...) {
		randGen.seed(std::random_device::result_type(std::time(nullptr)));
	}
	return randGen;
}
