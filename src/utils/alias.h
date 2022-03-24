#pragma once

#if (defined(__ANDROID__) || defined(__EMSCRIPTEN__)) && !defined(OPENGLES)
#define OPENGLES 1
#endif
#ifndef __ANDROID__
#define SDL_MAIN_HANDLED
#endif
#if defined(__ANDROID__) || defined(_WIN32) || defined(__APPLE__)
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif
#include <glm/glm.hpp>
#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string>
#include <tuple>
#include <unordered_map>
#include <unordered_set>
#include <vector>

using uchar = unsigned char;
using ushort = unsigned short;
using uint = unsigned int;
using ulong = unsigned long;
using ullong = unsigned long long;
using llong = long long;
using ldouble = long double;
using wchar = wchar_t;

using int8 = int8_t;
using uint8 = uint8_t;
using int16 = int16_t;
using uint16 = uint16_t;
using int32 = int32_t;
using uint32 = uint32_t;
using int64 = int64_t;
using uint64 = uint64_t;
using sizet = size_t;
using uptrt = uintptr_t;
using iptrt = intptr_t;
using pdift = ptrdiff_t;

using std::array;
using std::optional;
using std::pair;
using std::string;
using std::tuple;
using std::vector;
using std::wstring;

template <class T> using initlist = std::initializer_list<T>;
template <class... T> using sptr = std::shared_ptr<T...>;
template <class... T> using uptr = std::unique_ptr<T...>;
template <class... T> using umap = std::unordered_map<T...>;
template <class... T> using mumap = std::unordered_multimap<T...>;
template <class... T> using uset = std::unordered_set<T...>;
using pairStr = pair<string, string>;

using glm::mat3;
using glm::mat4;
using glm::quat;
using glm::vec2;
using glm::vec3;
using glm::vec4;
using glm::ivec2;
using glm::uvec2;
using glm::uvec4;
using svec2 = glm::vec<2, uint16, glm::defaultp>;

class AudioSys;
class Board;
class BoardObject;
class Button;
class CheckBox;
class ComboBox;
struct Config;
class Context;
class Icon;
class InputSys;
class Interactable;
class KeyGetter;
class Label;
class LabelEdit;
class Layout;
class Mesh;
class Navigator;
class Netcp;
class Object;
class Overlay;
class Piece;
class Popup;
class Program;
class ProgState;
class RootLayout;
class Scene;
class ScrollArea;
struct Settings;
struct Setup;
class ShaderBlur;
class ShaderBrights;
class ShaderDepth;
class ShaderFinal;
class ShaderGauss;
class ShaderGeom;
class ShaderGui;
class ShaderLight;
class ShaderSkybox;
class ShaderSsao;
class ShaderStartup;
class Slider;
class TextBox;
class Tile;
class TitleBar;
class Widget;
class WindowSys;

using BCall = void (Program::*)(Button*);
using GCall = void (Program::*)(BoardObject*, uint8);
using CCall = void (Program::*)(uint, const string&);
using SBCall = void (ProgState::*)();
using SACall = void (ProgState::*)(float);

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator++(T& a) {
	return a = T(std::underlying_type_t<T>(a) + std::underlying_type_t<T>(1));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator++(T& a, int) {
	T r = a;
	a = T(std::underlying_type_t<T>(a) + std::underlying_type_t<T>(1));
	return r;
}

template <class T, class U, std::enable_if_t<std::is_enum_v<T>&& std::is_integral_v<U>, int> = 0>
constexpr T operator+(T a, U b) {
	return T(std::underlying_type_t<T>(a) + std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator--(T& a) {
	return a = T(std::underlying_type_t<T>(a) - std::underlying_type_t<T>(1));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator--(T& a, int) {
	T r = a;
	a = T(std::underlying_type_t<T>(a) - std::underlying_type_t<T>(1));
	return r;
}

template <class T, class U, std::enable_if_t<std::is_enum_v<T>&& std::is_integral_v<U>, int> = 0>
constexpr T operator-(T a, U b) {
	return T(std::underlying_type_t<T>(a) - std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator~(T a) {
	return T(~std::underlying_type_t<T>(a));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator&(T a, T b) {
	return T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator&=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) & std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator|(T a, T b) {
	return T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator|=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) | std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator^(T a, T b) {
	return T(std::underlying_type_t<T>(a) ^ std::underlying_type_t<T>(b));
}

template <class T, std::enable_if_t<std::is_enum_v<T>, int> = 0>
constexpr T operator^=(T& a, T b) {
	return a = T(std::underlying_type_t<T>(a) ^ std::underlying_type_t<T>(b));
}

template <class T>
T readMem(const void* data) {
	T val;
	std::copy_n(static_cast<const uint8*>(data), sizeof(T), reinterpret_cast<uint8*>(&val));
	return val;
}

template <class T>
void* writeMem(void* data, const T& val) {
	return std::copy_n(reinterpret_cast<const uint8*>(&val), sizeof(T), static_cast<uint8*>(data));
}
