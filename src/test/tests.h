#include "engine/fileSys.h"
#include <iostream>

extern int testResult;

void testAlias();
void testFileSys();
void testServer();
void testText();
void testUtils();

bool operator!=(const IniLine& a, const IniLine& b);

template <class A, class B, class C, class D>
bool operator!=(const pair<A, B>& l, const pair<C, D>& r) {
	return l.first != r.first || l.second != r.second;
}

inline bool operator!=(const SDL_Rect& l, const SDL_Rect& r) {
	return l.x != r.x || l.y != r.y || l.w != r.w || l.h != r.h;
}

inline bool operator!=(const SDL_DisplayMode& l, const SDL_DisplayMode& r) {
	return !(l == r);
}

template <class A, class B>
std::ostream& operator<<(std::ostream& s, const pair<A, B>& p) {
	return s << '(' << p.first << ' ' << p.second << ')';
}

template <glm::length_t L, class T, glm::qualifier Q = glm::defaultp>
std::ostream& operator<<(std::ostream& s, const glm::vec<L, T, Q>& v) {
	s << '(';
	if constexpr (L == 1)
		return s << v[0] << ')';
	for (glm::length_t i = 0; i < L - 1; ++i)
		s << v[i] << ' ';
	return s << v[L - 1] << ')';
}

inline std::ostream& operator<<(std::ostream& s, const SDL_Rect& r) {
	return s << '(' << r.x << ' ' << r.y << ' ' << r.w << ' ' << r.h << ')';
}

inline std::ostream& operator<<(std::ostream& s, const SDL_DisplayMode& m) {
	return s << '(' << m.w << ' ' << m.h << ' ' << m.refresh_rate << ' ' << m.format << ' ' << m.driverdata << ')';
}

inline std::ostream& operator<<(std::ostream& s, const IniLine& l) {
	return s << "(\"" << l.type << "\" \"" << l.prp << "\" \"" << l.key << "\" \"" << l.val << "\")";
}

template <class T, class C = typename T::value_type>
void printRange(T beg, T end, const char* msg) {
	std::cerr << msg << ": ";
	if (beg != end) {
		for (; beg != end - 1; ++beg)
			std::cout << static_cast<C>(*beg) << ' ';
		std::cout << static_cast<C>(*beg);
	}
	std::cerr << linend;
}

template <class A, class B>
void assertEqualImpl(const A& res, const B& exp, const char* name) {
	if (res != exp) {
		testResult = EXIT_FAILURE;
		std::cerr << name << ':' << linend << "result: " << res << " == " << exp << linend << std::endl;
	}
}

template <class A, class B>
void assertNotEqualImpl(const A& res, const B& exp, const char* name) {
	if (res == exp) {
		testResult = EXIT_FAILURE;
		std::cerr << name << ':' << linend << "result: " << res << " != " << exp << linend << std::endl;
	}
}

template <class A, class B>
void assertLessImpl(const A& res, const B& exp, const char* name) {
	if (res >= exp) {
		testResult = EXIT_FAILURE;
		std::cerr << name << ':' << linend << "result: " << res << " < " << exp << linend << std::endl;
	}
}

template <class A, class B>
void assertLessEqualImpl(const A& res, const B& exp, const char* name) {
	if (res > exp) {
		testResult = EXIT_FAILURE;
		std::cerr << name << ':' << linend << "result: " << res << " <= " << exp << linend << std::endl;
	}
}

template <class A, class B>
void assertGreaterImpl(const A& res, const B& exp, const char* name) {
	if (res <= exp) {
		testResult = EXIT_FAILURE;
		std::cerr << name << ':' << linend << "result: " << res << " > " << exp << linend << std::endl;
	}
}

template <class A, class B>
void assertGreaterEqualImpl(const A& res, const B& exp, const char* name) {
	if (res < exp) {
		testResult = EXIT_FAILURE;
		std::cerr << name << ':' << linend << "result: " << res << " >= " << exp << linend << std::endl;
	}
}

template <class T>
void assertRangeImpl(const T& res, const T& exp, const char* name) {
	if (res != exp) {
		testResult = EXIT_FAILURE;
		std::cerr << name << ':' << linend;
		printRange(res.begin(), res.end(), "result");
		printRange(exp.begin(), exp.end(), "expect");
		std::cerr << std::endl;
	}
}

template <class T>
void assertMemoryImpl(const T* res, const T* exp, sizet cnt, const char* name) {
	if (!std::equal(res, res + cnt, exp)) {
		testResult = EXIT_FAILURE;
		std::cerr << name << ':' << linend;
		printRange<const T*, uint>(res, res + cnt, "result");
		printRange<const T*, uint>(exp, exp + cnt, "expect");
		std::cerr << std::endl;
	}
}

#define assertEqual(res, exp) assertEqualImpl(res, exp, __func__)
#define assertNotEqual(res, exp) assertNotEqualImpl(res, exp, __func__)
#define assertLess(res, exp) assertLessImpl(res, exp, __func__)
#define assertLessEqual(res, exp) assertLessEqualImpl(res, exp, __func__)
#define assertGreater(res, exp) assertGreaterImpl(res, exp, __func__)
#define assertGreaterEqual(res, exp) assertGreaterEqualImpl(res, exp, __func__)
#define assertTrue(res) assertEqual(res, true)
#define assertFalse(res) assertEqual(res, false)
#define assertRange(res, exp) assertRangeImpl(res, exp, __func__)
#define assertMemory(res, exp, cnt) assertMemoryImpl(res, exp, cnt, __func__)
