#pragma once

#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <math.h>

#include <algorithm>
#include <vector>
#include <array>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <optional>
#include <functional>

#include <fmt/chrono.h>
#include <fmt/color.h>
#include <fmt/format.h>
#include <fmt/ostream.h>

namespace std {
    namespace chrono {}
    inline namespace literals { inline namespace chrono_literals {} }
}

namespace chrono = std::chrono;

using namespace std::chrono_literals;
using namespace fmt::literals;

using String = std::string;
using StringView = std::string_view;

template<typename T>
using Array = std::vector<T>;
template<typename TKey, typename TValue, typename THash = std::hash<TKey>>
using HashMap = std::unordered_map<TKey, TValue, THash>;
template<typename TKey, typename THash = std::hash<TKey>>
using HashSet = std::unordered_set<TKey, THash>;
template<typename T>
using UniquePtr = std::unique_ptr<T>;
template<typename T>
using Optional = std::optional<T>;

template<typename T>
constexpr typename std::remove_reference<T>::type&& ToRvalue(T &&t) noexcept {
    return static_cast<typename std::remove_reference<T>::type &&>(t);
}


#include <glm/glm.hpp>
#include <glm/gtx/rotate_vector.hpp>
#include <glm/gtc/type_ptr.hpp>

#define VER_MAJOR 0
#define VER_MINOR 1
#define VER_BUILD 0

#define ARRAY_SIZE(arr) (sizeof(arr) / sizeof(arr[0]))

#define FAIL(message) \
    do { \
        fmt::print("Error: {}:{} ({}): {}\n", __FILE__, __LINE__, __func__, message); \
        std::abort(); \
    } while (false);

#define CHECK(expression) if (!(expression)) { FAIL(fmt::format("Check failed: {}", #expression)); }
#define UNREACHED         FAIL("Unreached code reached");
#define TODO              FAIL("Not implemented");

#define DUMP(x) log_info(#x, fmt::format("{}", x))

typedef bool     b8;
typedef uint32_t b32;
typedef int8_t   i8;
typedef int16_t  i16;
typedef int32_t  i32;
typedef int64_t  i64;
typedef uint8_t  u8;
typedef uint16_t u16;
typedef uint32_t u32;
typedef uint64_t u64;
typedef float    f32;
typedef double   f64;

using Vec2  = glm::vec2;
using Vec2i = glm::i32vec2;
using Vec2u = glm::u32vec2;
using Vec3  = glm::vec3;
using Vec4  = glm::vec4;
using Mat4  = glm::mat4;

template<typename T>
struct Rect {
    Rect() = default;

    constexpr Rect(T x, T y, T width, T height)
        : x(x)
        , y(y)
        , width(width)
        , height(height) {
    }

    template<typename TVec>
    constexpr Rect(TVec position, TVec size)
        : x(position.x)
        , y(position.y)
        , width(size.x)
        , height(size.y) {
    }

    T x;
    T y;
    T width;
    T height;
};

using FloatRect = Rect<f32>;

struct Tangent {
    constexpr Tangent(f32 m, f32 b)
        : m(m)
        , b(b) {
    };

    f32 m = 0;
    f32 b = 0;

    constexpr f32 f(f32 x) {
        return m * x + b;
    };
};

struct Color : public glm::u8vec4 {
    constexpr Color()
        : glm::u8vec4(255, 255, 255, 255) {
    }

    template<typename R, typename G, typename B, typename A>
    constexpr Color(R r, G g, B b, A a = 255)
        : glm::u8vec4(r, g, b, a) {
    }
};

template<class... Ts>
struct Overloaded : Ts...  {
    using Ts::operator()...;
};

template<class... Ts>
Overloaded(Ts...) -> Overloaded<Ts...>;


template <typename F>
struct ScopeExit {
    F f;
    ScopeExit(F f) : f(f) {}
    ~ScopeExit() { f(); }
};

struct DEFER_TAG {};

template<class F>
ScopeExit<F> operator+(DEFER_TAG, F &&f) {
    return ScopeExit<F>{std::forward<F>(f)};
}

#define DEFER_1(x, y) x##y
#define DEFER_2(x, y) DEFER_1(x, y)
#define defer auto DEFER_2(ScopeExit, __LINE__) = DEFER_TAG{} + [&]()
