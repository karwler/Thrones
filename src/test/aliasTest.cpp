#include "tests.h"

static void testReadMem() {
	uint8 mem[] = { 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08 };
	assertEqual(readMem<uint8>(mem), 0x10u);
	assertEqual(readMem<uint16>(mem), 0x2010u);
	assertEqual(readMem<uint32>(mem), 0x80402010u);
	assertEqual(readMem<uint64>(mem), 0x0804020180402010u);
}

static void testWriteMem() {
	uint8 mem[8], exp[] = { 0x10, 0x20, 0x40, 0x80, 0x1, 0x2, 0x4, 0x8 };
	writeMem(mem, 0x10_ub);
	assertMemory(mem, exp, 1);
	writeMem(mem, 0x2010_us);
	assertMemory(mem, exp, 2);
	writeMem(mem, 0x80402010u);
	assertMemory(mem, exp, 4);
	writeMem(mem, 0x0804020180402010ull);
	assertMemory(mem, exp, 8);
}

void testAlias() {
	puts("Running Alias tests...");
	testReadMem();
	testWriteMem();
}
