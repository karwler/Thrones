#include "tests.h"
#include "server/server.h"

static void testWsKey() {
	assertEqual(Com::encodeBase64(Com::digestSha1("dGhlIHNhbXBsZSBub25jZQ==258EAFA5-E914-47DA-95CA-C5AB0DC85B11")), "s3pPLMBiTxaQ9kYGzzhZRbK+xOo=");
	assertEqual(Com::encodeBase64(Com::digestSha1("xqBt3ImNzJbYqRINxEFlkg==258EAFA5-E914-47DA-95CA-C5AB0DC85B11")), "K7DJLdLooIwIG/MOpvWFB3y3FE8=");
	assertEqual(Com::encodeBase64(Com::digestSha1("x3JJHMbDL1EzLkh9GBhXDw==258EAFA5-E914-47DA-95CA-C5AB0DC85B11")), "HSmrc0sMlYUkAGmm5OPpG2HaGWk=");
	assertEqual(Com::encodeBase64(Com::digestSha1("Iv8io/9s+lYFgZWcXczP8Q==258EAFA5-E914-47DA-95CA-C5AB0DC85B11")), "hsBlbuDTkk24srzEOTBUlZAlC2g=");
}

static void testReadCom() {
	uint8 mem[] = { 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08 };
	assertEqual(Com::read16(mem), 0x1020u);
	assertEqual(Com::read32(mem), 0x10204080u);
	assertEqual(Com::read64(mem), 0x1020408001020408u);
}

static void testWriteCom() {
	uint8 mem[8], exp[] = { 0x10, 0x20, 0x40, 0x80, 0x01, 0x02, 0x04, 0x08 };
	Com::write16(mem, 0x1020);
	assertMemory(mem, exp, 2);
	Com::write32(mem, 0x10204080);
	assertMemory(mem, exp, 4);
	Com::write64(mem, 0x1020408001020408);
	assertMemory(mem, exp, 8);
}

static void testReadText() {
	uint8 none[] = { 0, 0, 3 }, letter[] = { 0, 0, 4, 'a' }, word[] = { 0, 0, 8, 'h', 'e', 'l', 'l', 'o' };
	assertEqual(Com::readText(none), "");
	assertEqual(Com::readText(letter), "a");
	assertEqual(Com::readText(word), "hello");
}

static void testReadName() {
	uint8 none[] = { 0 }, letter[] = { 1, 'a' }, word[] = { 5, 'h', 'e', 'l', 'l', 'o' };
	assertEqual(Com::readName(none), "");
	assertEqual(Com::readName(letter), "a");
	assertEqual(Com::readName(word), "hello");
}

static void testBufferPush() {
	Com::Buffer b;
	b.pushHead(Com::Code(-1), 19);
	b.push(uint8(0x10));
	b.push({ uint8(0x20), uint8(0x40), uint8(0x80) });
	b.push(uint16(0x0201));
	b.push({ uint16(0x0402), uint16(0x0804), uint16(0x0108) });
	b.push("name");

	uint8 exp[] = { 0xFF, 0, 19, 0x10, 0x20, 0x40, 0x80, 0x02, 0x01, 0x04, 0x02, 0x08, 0x04, 0x01, 0x08, 'n', 'a', 'm', 'e' };
	assertEqual(b.getDlim(), 19u);
	assertMemory(&b[0], exp, 19);

	Com::Buffer c = std::move(b);
	assertEqual(b.getData(), nullptr);
	assertEqual(c.getDlim(), 19u);
	assertMemory(&c[0], exp, 19);
}

static void testBufferWrite() {
	Com::Buffer b;
	b.allocate(Com::Code(-1), 6);
	b.write(uint8(0x10), 3);
	b.write(uint16(0x2040), 4);

	uint8 exp[] = { 0xFF, 0, 6, 0x10, 0x20, 0x40 };
	assertEqual(b.getDlim(), 6u);
	assertMemory(&b[0], exp, 6);

	Com::Buffer c = std::move(b);
	assertEqual(b.getData(), nullptr);
	assertEqual(c.getDlim(), 6u);
	assertMemory(&c[0], exp, 6);
}

void testServer() {
	puts("Running Server tests...");
	testWsKey();
	testReadCom();
	testWriteCom();
	testReadText();
	testReadName();
	testBufferPush();
	testBufferWrite();
}
