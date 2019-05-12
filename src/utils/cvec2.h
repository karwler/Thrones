#pragma once

#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <algorithm>
#include <cmath>
#include <string>

using glm::mat4;
using glm::vec2;
using glm::vec3;
using glm::vec4;

using sizet = size_t;
using pdift = ptrdiff_t;

using std::string;
using std::wstring;
using std::to_string;

template <class T>
struct cvec2 {
	union { T x, w, u, b; };
	union { T y, h, v, t; };

	cvec2() = default;
	template <class A> constexpr cvec2(const A& n);
	template <class A, class B> constexpr cvec2(const A& x, const B& y);
	template <class A> constexpr cvec2(const cvec2<A>& v);

	T& operator[](sizet i);
	constexpr const T& operator[](sizet i) const;

	cvec2& operator+=(const cvec2& v);
	cvec2& operator-=(const cvec2& v);
	cvec2& operator*=(const cvec2& v);
	cvec2& operator/=(const cvec2& v);
	cvec2& operator%=(const cvec2& v);
	cvec2& operator&=(const cvec2& v);
	cvec2& operator|=(const cvec2& v);
	cvec2& operator^=(const cvec2& v);
	cvec2& operator<<=(const cvec2& v);
	cvec2& operator>>=(const cvec2& v);
	cvec2& operator++();
	cvec2 operator++(int);
	cvec2& operator--();
	cvec2 operator--(int);

	template <class F, class... A> static cvec2 get(const string& str, F strtox, A... args);
	string toString(char sep = ' ') const;
	constexpr vec2 glm() const;
	constexpr bool has(const T& n) const;
	constexpr bool hasNot(const T& n) const;
	constexpr T length() const;
	constexpr T area() const;
	constexpr T ratio() const;
	constexpr cvec2 swap() const;
	cvec2 swap(bool yes) const;
	static cvec2 swap(const T& x, const T& y, bool swap);
	constexpr cvec2 clamp(const cvec2& min, const cvec2& max) const;
};

template <class T> template <class A>
constexpr cvec2<T>::cvec2(const A& n) :
	x(T(n)),
	y(T(n))
{}

template <class T> template <class A, class B>
constexpr cvec2<T>::cvec2(const A& x, const B& y) :
	x(T(x)),
	y(T(y))
{}

template <class T> template <class A>
constexpr cvec2<T>::cvec2(const cvec2<A>& v) :
	x(T(v.x)),
	y(T(v.y))
{}

template <class T>
T& cvec2<T>::operator[](sizet i) {
	return (&x)[i];
}

template <class T>
constexpr const T& cvec2<T>::operator[](sizet i) const {
	return (&x)[i];
}

template <class T>
cvec2<T>& cvec2<T>::operator+=(const cvec2& v) {
	x += v.x;
	y += v.y;
	return *this;
}

template <class T>
cvec2<T>& cvec2<T>::operator-=(const cvec2& v) {
	x -= v.x;
	y -= v.y;
	return *this;
}

template <class T>
cvec2<T>& cvec2<T>::operator*=(const cvec2& v) {
	x *= v.x;
	y *= v.y;
	return *this;
}

template <class T>
cvec2<T>& cvec2<T>::operator/=(const cvec2& v) {
	x /= v.x;
	y /= v.y;
	return *this;
}

template <class T>
cvec2<T>& cvec2<T>::operator%=(const cvec2& v) {
	x %= v.x;
	y %= v.y;
	return *this;
}

template <class T>
cvec2<T>& cvec2<T>::operator&=(const cvec2& v) {
	x &= v.x;
	y &= v.y;
	return *this;
}

template <class T>
cvec2<T>& cvec2<T>::operator|=(const cvec2& v) {
	x |= v.x;
	y |= v.y;
	return *this;
}

template <class T>
cvec2<T>& cvec2<T>::operator^=(const cvec2& v) {
	x ^= v.x;
	y ^= v.y;
	return *this;
}

template <class T>
cvec2<T>& cvec2<T>::operator<<=(const cvec2& v) {
	x <<= v.x;
	y <<= v.y;
	return *this;
}

template <class T>
cvec2<T>& cvec2<T>::operator>>=(const cvec2& v) {
	x >>= v.x;
	y >>= v.y;
	return *this;
}

template <class T>
cvec2<T>& cvec2<T>::operator++() {
	x++;
	y++;
	return *this;
}

template <class T>
cvec2<T> cvec2<T>::operator++(int) {
	cvec2 t = *this;
	x++;
	y++;
	return t;
}

template <class T>
cvec2<T>& cvec2<T>::operator--() {
	x--;
	y--;
	return *this;
}

template <class T>
cvec2<T> cvec2<T>::operator--(int) {
	cvec2 t = *this;
	x--;
	y--;
	return t;
}

template <class T> template <class F, class... A>
cvec2<T> cvec2<T>::get(const string& str, F strtox, A... args) {
	cvec2<T> vec(T(0));
	const char* pos = str.c_str();
	for (unsigned int i = 0; *pos && i < 2; i++) {
		char* end;
		if (T num = T(strtox(pos, &end, args...)); end != pos) {
			vec[i] = num;
			pos = end;
		} else
			pos++;
	}
	return vec;
}

template<class T>
inline string cvec2<T>::toString(char sep) const {
	return to_string(x) + sep + to_string(y);
}

template<class T>
constexpr vec2 cvec2<T>::glm() const {
	return vec2(float(x), float(y));
}

template <class T>
constexpr bool cvec2<T>::has(const T& n) const {
	return x == n || y == n;
}

template <class T>
constexpr bool cvec2<T>::hasNot(const T& n) const {
	return x != n && y != n;
}

template <class T>
constexpr T cvec2<T>::length() const {
	return std::sqrt(x*x + y*y);
}

template<class T>
constexpr T cvec2<T>::area() const {
	return x * y;
}

template<class T>
constexpr T cvec2<T>::ratio() const {
	return x / y;
}

template <class T>
constexpr cvec2<T> cvec2<T>::swap() const {
	return cvec2(y, x);
}

template <class T>
cvec2<T> cvec2<T>::swap(bool yes) const {
	return yes ? swap() : *this;
}

template <class T>
cvec2<T> cvec2<T>::swap(const T& x, const T& y, bool swap) {
	return swap ? cvec2<T>(y, x) : cvec2<T>(x, y);
}

template<class T>
constexpr cvec2<T> cvec2<T>::clamp(const cvec2<T>& min, const cvec2<T>& max) const {
	return cvec2<T>(std::clamp(x, min.x, max.x), std::clamp(y, min.y, max.y));
}

template <class T>
constexpr cvec2<T> operator+(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x + b.x, a.y + b.y);
}

template <class T>
constexpr cvec2<T> operator+(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x + b, a.y + b);
}

template <class T>
constexpr cvec2<T> operator+(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a + b.x, a + b.y);
}

template <class T>
constexpr cvec2<T> operator-(const cvec2<T>& a) {
	return cvec2<T>(-a.x, -a.y);
}

template <class T>
constexpr cvec2<T> operator-(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x - b.x, a.y - b.y);
}

template <class T>
constexpr cvec2<T> operator-(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x - b, a.y - b);
}

template <class T>
constexpr cvec2<T> operator-(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a - b.x, a - b.y);
}

template <class T>
constexpr cvec2<T> operator*(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x * b.x, a.y * b.y);
}

template <class T>
constexpr cvec2<T> operator*(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x * b, a.y * b);
}

template <class T>
constexpr cvec2<T> operator*(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a * b.x, a * b.y);
}

template <class T>
constexpr cvec2<T> operator/(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x / b.x, a.y / b.y);
}

template <class T>
constexpr cvec2<T> operator/(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x / b, a.y / b);
}

template <class T>
constexpr cvec2<T> operator/(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a / b.x, a / b.y);
}

template <class T>
constexpr cvec2<T> operator%(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x % b.x, a.y % b.y);
}

template <class T>
constexpr cvec2<T> operator%(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x % b, a.y % b);
}

template <class T>
constexpr cvec2<T> operator%(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a % b.x, a % b.y);
}

template <class T>
constexpr cvec2<T> operator~(const cvec2<T>& a) {
	return cvec2<T>(~a.x, ~a.y);
}

template <class T>
constexpr cvec2<T> operator&(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x & b.x, a.y & b.y);
}

template <class T>
constexpr cvec2<T> operator&(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x & b, a.y & b);
}

template <class T>
constexpr cvec2<T> operator&(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a & b.x, a & b.y);
}

template <class T>
constexpr cvec2<T> operator|(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x | b.x, a.y | b.y);
}

template <class T>
constexpr cvec2<T> operator|(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x | b, a.y | b);
}

template <class T>
constexpr cvec2<T> operator|(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a | b.x, a | b.y);
}

template <class T>
constexpr cvec2<T> operator^(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x ^ b.x, a.y ^ b.y);
}

template <class T>
constexpr cvec2<T> operator^(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x ^ b, a.y ^ b);
}

template <class T>
constexpr cvec2<T> operator^(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a ^ b.x, a ^ b.y);
}

template <class T>
constexpr bool operator==(const cvec2<T>& a, const cvec2<T>& b) {
	return a.x == b.x && a.y == b.y;
}

template <class T>
constexpr bool operator==(const cvec2<T>& a, const T& b) {
	return a.x == b && a.y == b;
}

template <class T>
constexpr bool operator==(const T& a, const cvec2<T>& b) {
	return a == b.x && a == b.y;
}

template <class T>
constexpr bool operator!=(const cvec2<T>& a, const cvec2<T>& b) {
	return a.x != b.x || a.y != b.y;
}

template <class T>
constexpr bool operator!=(const cvec2<T>& a, const T& b) {
	return a.x != b || a.y != b;
}

template <class T>
constexpr bool operator!=(const T& a, const cvec2<T>& b) {
	return a != b.x || a != b.y;
}
