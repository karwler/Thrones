#pragma once

#include <algorithm>
#include <cmath>
#include <string>

using std::string;
using std::wstring;
using std::to_string;

template <class T>
struct cvec2 {
	union { T x, w, u, b; };
	union { T y, h, v, t; };

	cvec2();
	template <class A> cvec2(const A& n);
	template <class A, class B> cvec2(const A& x, const B& y);
	template <class A, class B> cvec2(const A& vx, const B& vy, bool swap);
	template <class A> cvec2(const cvec2<A>& v);
	template <class A> cvec2(const cvec2<A>& v, bool swap);
#ifdef _MSC_VER
	~cvec2() {}
#endif

	T& operator[](size_t i);
	const T& operator[](size_t i) const;

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

	template <class F, class... A> cvec2& set(const string& str, F strtox, A... args);
	string toString() const;
	bool has(const T& n) const;
	bool hasNot(const T& n) const;
	T length() const;
	cvec2 swap() const;
	cvec2 swap(bool yes) const;
	cvec2 clamp(const cvec2& min, const cvec2& max) const;
};

template <class T>
cvec2<T>::cvec2() :
	x(T(0)),
	y(T(0))
{}

template <class T> template <class A>
cvec2<T>::cvec2(const A& n) :
	x(T(n)),
	y(T(n))
{}

template <class T> template <class A, class B>
cvec2<T>::cvec2(const A& x, const B& y) :
	x(T(x)),
	y(T(y))
{}

template <class T> template <class A, class B>
cvec2<T>::cvec2(const A& vx, const B& vy, bool swap) {
	if (swap) {
		y = T(vx);
		x = T(vy);
	} else {
		y = T(vy);
		x = T(vx);
	}
}

template <class T> template <class A>
cvec2<T>::cvec2(const cvec2<A>& v) :
	x(T(v.x)),
	y(T(v.y))
{}

template <class T> template <class A>
cvec2<T>::cvec2(const cvec2<A>& v, bool swap) {
	if (swap) {
		y = T(v.x);
		x = T(v.y);
	} else {
		y = T(v.y);
		x = T(v.x);
	}
}

template <class T>
T& cvec2<T>::operator[](size_t i) {
	return (&x)[i];
}

template <class T>
const T& cvec2<T>::operator[](size_t i) const {
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
cvec2<T>& cvec2<T>::set(const string& str, F strtox, A... args) {
	const char* pos = str.c_str();
	for (; *pos > '\0' && *pos <= ' '; pos++);

	for (unsigned int i = 0; *pos && i < 2; i++) {
		char* end;
		if (T num = T(strtox(pos, &end, args...)); end != pos) {
			(*this)[i] = num;
			for (pos = end; *pos > '\0' && *pos <= ' '; pos++);
		} else
			pos++;
	}
	return *this;
}

template<class T>
inline string cvec2<T>::toString() const {
	return to_string(x) + ' ' + to_string(y);
}

template <class T>
bool cvec2<T>::has(const T& n) const {
	return x == n || y == n;
}

template <class T>
bool cvec2<T>::hasNot(const T& n) const {
	return x != n && y != n;
}

template <class T>
T cvec2<T>::length() const {
	return std::sqrt(x*x + y*y);
}

template <class T>
cvec2<T> cvec2<T>::swap() const {
	return cvec2(y, x);
}

template <class T>
cvec2<T> cvec2<T>::swap(bool yes) const {
	return yes ? swap() : *this;
}

template<class T>
cvec2<T> cvec2<T>::clamp(const cvec2<T>& min, const cvec2<T>& max) const {
	return cvec2<T>(std::clamp(x, min.x, max.x), std::clamp(y, min.y, max.y));
}

template <class T>
cvec2<T> operator+(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x + b.x, a.y + b.y);
}

template <class T>
cvec2<T> operator+(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x + b, a.y + b);
}

template <class T>
cvec2<T> operator+(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a + b.x, a + b.y);
}

template <class T>
cvec2<T> operator-(const cvec2<T>& a) {
	return cvec2<T>(-a.x, -a.y);
}

template <class T>
cvec2<T> operator-(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x - b.x, a.y - b.y);
}

template <class T>
cvec2<T> operator-(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x - b, a.y - b);
}

template <class T>
cvec2<T> operator-(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a - b.x, a - b.y);
}

template <class T>
cvec2<T> operator*(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x * b.x, a.y * b.y);
}

template <class T>
cvec2<T> operator*(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x * b, a.y * b);
}

template <class T>
cvec2<T> operator*(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a * b.x, a * b.y);
}

template <class T>
cvec2<T> operator/(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x / b.x, a.y / b.y);
}

template <class T>
cvec2<T> operator/(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x / b, a.y / b);
}

template <class T>
cvec2<T> operator/(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a % b.x, a % b.y);
}

template <class T>
cvec2<T> operator%(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x % b.x, a.y % b.y);
}

template <class T>
cvec2<T> operator%(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x % b, a.y % b);
}

template <class T>
cvec2<T> operator%(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a % b.x, a % b.y);
}

template <class T>
cvec2<T> operator~(const cvec2<T>& a) {
	return cvec2<T>(~a.x, ~a.y);
}

template <class T>
cvec2<T> operator&(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x & b.x, a.y & b.y);
}

template <class T>
cvec2<T> operator&(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x & b, a.y & b);
}

template <class T>
cvec2<T> operator&(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a & b.x, a & b.y);
}

template <class T>
cvec2<T> operator|(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x | b.x, a.y | b.y);
}

template <class T>
cvec2<T> operator|(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x | b, a.y | b);
}

template <class T>
cvec2<T> operator|(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a | b.x, a | b.y);
}

template <class T>
cvec2<T> operator^(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x ^ b.x, a.y ^ b.y);
}

template <class T>
cvec2<T> operator^(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x ^ b, a.y ^ b);
}

template <class T>
cvec2<T> operator^(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a ^ b.x, a ^ b.y);
}

template <class T>
cvec2<T> operator<<(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x << b.x, a.y << b.y);
}

template <class T>
cvec2<T> operator<<(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x << b, a.y << b);
}

template <class T>
cvec2<T> operator<<(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a << b.x, a << b.y);
}

template <class T>
cvec2<T> operator>>(const cvec2<T>& a, const cvec2<T>& b) {
	return cvec2<T>(a.x >> b.x, a.y >> b.y);
}

template <class T>
cvec2<T> operator>>(const cvec2<T>& a, const T& b) {
	return cvec2<T>(a.x >> b, a.y >> b);
}

template <class T>
cvec2<T> operator>>(const T& a, const cvec2<T>& b) {
	return cvec2<T>(a >> b.x, a >> b.y);
}

template <class T>
bool operator==(const cvec2<T>& a, const cvec2<T>& b) {
	return a.x == b.x && a.y == b.y;
}

template <class T>
bool operator==(const cvec2<T>& a, const T& b) {
	return a.x == b && a.y == b;
}

template <class T>
bool operator==(const T& a, const cvec2<T>& b) {
	return a == b.x && a == b.y;
}

template <class T>
bool operator!=(const cvec2<T>& a, const cvec2<T>& b) {
	return a.x != b.x || a.y != b.y;
}

template <class T>
bool operator!=(const cvec2<T>& a, const T& b) {
	return a.x != b || a.y != b;
}

template <class T>
bool operator!=(const T& a, const cvec2<T>& b) {
	return a != b.x || a != b.y;
}
