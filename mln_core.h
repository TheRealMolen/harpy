#pragma once

#include <concepts>
#include <cstdint>

#include "Utility/dsp.h"

#ifdef _MSC_VER
// msvc
#include <malloc.h>
#endif

//-------------------------------------------------------------------------------------------------------------

using i8 = int8_t;
using i16 = int16_t;
using i32 = int32_t;
using i64 = int64_t;
using u8  = uint8_t;
using u16 = uint16_t;
using u32 = uint32_t;
using u64 = uint64_t;

using uint = unsigned int;

using std::pair;
using std::begin;
using std::end;

using daisysp::fclamp;


constexpr float kPi = PI_F;
constexpr float kTwoPi = kPi * 2.0f;

constexpr float kEpsilon = 0.00001f;

//-------------------------------------------------------------------------------------------------------------

template<unsigned kPowerOfTwo, typename T>
requires std::unsigned_integral<T>
constexpr inline T round_up_2n(T val)
{
    static_assert(T(kPowerOfTwo) == kPowerOfTwo);
    static_assert((kPowerOfTwo & (kPowerOfTwo - 1)) == 0, "kPowerOfTwo isn't a power of two");

    const T po2 = kPowerOfTwo;
    return ((val + po2 - 1) & ~(po2 - 1));
}

template<typename T>
requires std::unsigned_integral<T>
constexpr inline T round_up_2n(T val, u32 powerOfTwo)
{
    const T po2 = powerOfTwo;
    return ((val + po2 - 1) & ~(po2 - 1));
}

//-------------------------------------------------------------------------------------------------------------

#define count_of(_arr)  (sizeof((_arr)) / sizeof((_arr)[0]))

//-------------------------------------------------------------------------------------------------------------

template<typename T>
requires std::integral<T> || std::floating_point<T>
inline constexpr T square(T v)
{
    return v * v;
}

//-------------------------------------------------------------------------------------------------------------

inline bool isSimilar(float a, float b, float epsilon=kEpsilon)
{
    return fabsf(a - b) <= epsilon;
}

//-------------------------------------------------------------------------------------------------------------

using std::clamp;

// note: daisytoolchain\arm-none-eabi\include\c++\10.2.1\math.h (glibc++ math.h) pulls std::lerp into global ns :(
constexpr inline float flerp(float a, float b, float t)
{
	return a + (b-a) * t;
}

//-------------------------------------------------------------------------------------------------------------

inline float toRads(float degrees)
{
    return degrees * (kPi / 180.f);
}

//-------------------------------------------------------------------------------------------------------------
