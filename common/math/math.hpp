/*
 * math.hpp
 * Copyright (c) 2021, Kirill GPRB.
 * All Rights Reserved.
 */
#pragma once
#include <common/math/const.hpp>
#include <type_traits>

namespace math
{
template<typename T>
constexpr static inline const T log2(const T x)
{
    if(x < 2)
        return 0;
    return math::log2<T>((x + 1) >> 1) + 1;
}

template<typename T, typename F>
constexpr static inline const T ceil(const F x)
{
    static_assert(std::is_integral_v<T>);
    static_assert(std::is_floating_point_v<F>);
    const T ival = static_cast<T>(x);
    if(ival == static_cast<F>(ival))
        return ival;
    return ival + ((x > 0) ? 1 : 0);
}

template<typename T>
constexpr static inline const T pow2(const T x)
{
    T value = 1;
    while(value < x)
        value *= 2;
    return value;
}

template<typename T>
constexpr static inline const T min(const T x, const T y)
{
    if(x > y)
        return y;
    return x;
}

template<typename T>
constexpr static inline const T max(const T x, const T y)
{
    if(x < y)
        return y;
    return x;
}

template<typename T>
constexpr static inline const T clamp(const T x, const T min, const T max)
{
    if(x < min)
        return min;
    if(x > max)
        return max;
    return x;
}

static inline const float wrapAngle180N(const float angle)
{
    const float wrap = glm::mod(angle + ANGLE_180D, ANGLE_360D);
    return ((wrap < 0.0f) ? (wrap + ANGLE_360D) : wrap) - ANGLE_180D;
}

static inline const float wrapAngle180P(const float angle)
{
    return glm::mod(glm::mod(angle, ANGLE_180D) + ANGLE_180D, ANGLE_180D);
}

static inline const float wrapAngle360P(const float angle)
{
    return glm::mod(glm::mod(angle, ANGLE_360D) + ANGLE_360D, ANGLE_360D);
}

template<glm::length_t L, glm::qualifier Q>
static inline const glm::vec<L, float, Q> wrapAngle180N(const glm::vec<L, float, Q> &angles)
{
    glm::vec<L, float, Q> wrap;
    for(glm::length_t i = 0; i < L; i++)
        wrap[i] = math::wrapAngle180N(angles[i]);
    return wrap;
}

template<glm::length_t L, glm::qualifier Q>
static inline const glm::vec<L, float, Q> wrapAngle360P(const glm::vec<L, float, Q> &angles)
{
    glm::vec<L, float, Q> wrap;
    for(glm::length_t i = 0; i < L; i++)
        wrap[i] = math::wrapAngle360P(angles[i]);
    return wrap;
}

template<typename T, size_t L>
constexpr static inline const size_t arraySize(T(&)[L])
{
    return L;
}

constexpr static inline const bool isInBB(const float3 &p, const float3 &a, const float3 &b)
{
    return p.x >= a.x && p.y >= a.y && p.z >= a.z && p.x <= b.x && p.y <= b.y && p.z <= b.z;
}

template<typename T>
constexpr static inline void vecToArray(const T &vec, typename T::value_type array[T::length()])
{
    for(typename T::length_type i = 0; i < T::length(); i++)
        array[i] = vec[i];
}

template<typename T>
constexpr static inline const T arrayToVec(const typename T::value_type array[T::length()])
{
    T vec;
    for(typename T::length_type i = 0; i < T::length(); i++)
        vec[i] = array[i];
    return vec;
}
} // namespace math
