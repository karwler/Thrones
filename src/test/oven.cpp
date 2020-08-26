#include "tests.h"

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

void testOven() {
	puts("Running Oven tests...");
	testCycle();
}
