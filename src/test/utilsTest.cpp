#include "tests.h"

static void testUniqueSort() {
	vector<int> vals{ 4, 8, 2, 5, 5, 0, 9, 6, 7, 2, 1, 4, 3 };
	assertRange(uniqueSort(vals), vector<int>({ 0, 1, 2, 3, 4, 5, 6, 7, 8, 9 }));
}

static void testSortNames() {
	umap<string, int> vals = { pair("a01", 0), pair("zb", 0), pair("a1", 0), pair("Zb", 0), pair("a10", 0) };
	assertRange(sortNames(vals), vector<string>({ "a01", "a1", "a10", "Zb", "zb" }));
}

static void testDirection() {
	assertTrue(Direction(Direction::up).vertical());
	assertTrue(Direction(Direction::up).negative());
	assertTrue(Direction(Direction::down).vertical());
	assertTrue(Direction(Direction::down).positive());
	assertTrue(Direction(Direction::left).horizontal());
	assertTrue(Direction(Direction::left).negative());
	assertTrue(Direction(Direction::right).horizontal());
	assertTrue(Direction(Direction::right).positive());
}

static void testBtom() {
	assertEqual(btom<int>(true), 1);
	assertEqual(btom<int>(false), -1);
}

static void testInRange() {
	assertTrue(inRange(0, -1, 1));
	assertTrue(inRange(0, 0, 1));
	assertFalse(inRange(0, 0, 0));
	assertFalse(inRange(1, 0, 1));
	assertFalse(inRange(0, 1, 2));
	assertTrue(inRange(ivec2(0), ivec2(-1), ivec2(1)));
	assertTrue(inRange(ivec2(0), ivec2(0), ivec2(1)));
	assertFalse(inRange(ivec2(0), ivec2(0), ivec2(0)));
	assertFalse(inRange(ivec2(1), ivec2(0), ivec2(1)));
	assertFalse(inRange(ivec2(0), ivec2(1), ivec2(2)));
}

static void testOutRange() {
	assertTrue(outRange(-1, 0, 1));
	assertTrue(outRange(1, 0, 1));
	assertTrue(outRange(0, 0, 0));
	assertFalse(outRange(0, 0, 1));
	assertFalse(outRange(1, 0, 2));
	assertTrue(outRange(ivec2(-1), ivec2(0), ivec2(1)));
	assertTrue(outRange(ivec2(1), ivec2(0), ivec2(1)));
	assertTrue(outRange(ivec2(0), ivec2(0), ivec2(0)));
	assertFalse(outRange(ivec2(0), ivec2(0), ivec2(1)));
	assertFalse(outRange(ivec2(1), ivec2(0), ivec2(2)));
}

static void testSwap() {
	assertEqual(swap(0, 0, false), ivec2(0));
	assertEqual(swap(0, 0, true), ivec2(0));
	assertEqual(swap(4, 0, false), ivec2(4, 0));
	assertEqual(swap(4, 0, true), ivec2(0, 4));
	assertEqual(swap(0, -3, false), ivec2(0, -3));
	assertEqual(swap(0, -3, true), ivec2(-3, 0));
	assertEqual(swap(3, 5, false), ivec2(3, 5));
	assertEqual(swap(3, 5, true), ivec2(5, 3));
}

static void testDeltaSingle() {
	assertEqual(deltaSingle(ivec2(0)), ivec2(0));
	assertEqual(deltaSingle(ivec2(12, 0)), ivec2(1, 0));
	assertEqual(deltaSingle(ivec2(-21, 0)), ivec2(-1, 0));
	assertEqual(deltaSingle(ivec2(0, 45)), ivec2(0, 1));
	assertEqual(deltaSingle(ivec2(0, -54)), ivec2(0, -1));
	assertEqual(deltaSingle(ivec2(-2, 1)), ivec2(-1, 1));
	assertEqual(deltaSingle(ivec2(-1, 2)), ivec2(-1, 1));
	assertEqual(deltaSingle(ivec2(1, -1)), ivec2(1, -1));
	assertEqual(deltaSingle(ivec2(2, -2)), ivec2(1, -1));
}

static void testSwapBits() {
	assertEqual(swapBits(0u, 0, 0), 0u);
	assertEqual(swapBits(0u, 0, 1), 0u);
	assertEqual(swapBits(0u, 3, 2), 0u);
	assertEqual(swapBits(1u, 0, 0), 1u);
	assertEqual(swapBits(1u, 0, 1), 2u);
	assertEqual(swapBits(1u, 1, 0), 2u);
	assertEqual(swapBits(2u, 0, 1), 1u);
	assertEqual(swapBits(2u, 1, 0), 1u);
	assertEqual(swapBits(3u, 0, 1), 3u);
	assertEqual(swapBits(3u, 1, 0), 3u);
	assertEqual(swapBits(0xF0u, 0, 0), 0xF0u);
	assertEqual(swapBits(0xF0u, 0, 1), 0xF0u);
	assertEqual(swapBits(0xF0u, 3, 2), 0xF0u);
	assertEqual(swapBits(0xF1u, 0, 0), 0xF1u);
	assertEqual(swapBits(0xF1u, 0, 1), 0xF2u);
	assertEqual(swapBits(0xF1u, 1, 0), 0xF2u);
	assertEqual(swapBits(0xF2u, 0, 1), 0xF1u);
	assertEqual(swapBits(0xF2u, 1, 0), 0xF1u);
	assertEqual(swapBits(0xF3u, 0, 1), 0xF3u);
	assertEqual(swapBits(0xF3u, 1, 0), 0xF3u);
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

static void testCeilPower2() {
	assertEqual(ceilPower2(0), 0u);
	assertEqual(ceilPower2(1), 1u);
	assertEqual(ceilPower2(2), 2u);
	assertEqual(ceilPower2(3), 4u);
	assertEqual(ceilPower2(4), 4u);
	assertEqual(ceilPower2(12), 16u);
	assertEqual(ceilPower2(16), 16u);
	assertEqual(ceilPower2(24), 32u);
	assertEqual(ceilPower2(32), 32u);
	assertEqual(ceilPower2(1000), 1024u);
	assertEqual(ceilPower2(1024), 1024u);
}

void testUtils() {
	puts("Running Utils tests...");
	testUniqueSort();
	testSortNames();
	testDirection();
	testBtom();
	testInRange();
	testOutRange();
	testSwap();
	testDeltaSingle();
	testSwapBits();
	testCycle();
	testCeilPower2();
}
