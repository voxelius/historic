/*
 * object.hpp
 * Copyright (c) 2021, Kirill GPRB.
 * All Rights Reserved.
 */
#pragma once
#include <common/traits.hpp>
#include <glad/gl.h>
#include <stddef.h>
#include <stdint.h>
#include <utility>

namespace gl
{
template<typename T>
class Object : public NonCopyable {
public:
    Object() = default;
    virtual ~Object();
    void create();
    void destroy();
    constexpr bool valid() const;
    constexpr uint32_t get() const;

protected:
    uint32_t handle { 0 };
};
} // namespace gl

template<typename T>
inline gl::Object<T>::~Object()
{
    destroy();
}

template<typename T>
inline void gl::Object<T>::create()
{
    static_cast<T *>(this)->create();
}

template<typename T>
inline void gl::Object<T>::destroy()
{
    static_cast<T *>(this)->destroy();
}

template<typename T>
inline constexpr bool gl::Object<T>::valid() const
{
    return handle != 0;
}

template<typename T>
inline constexpr uint32_t gl::Object<T>::get() const
{
    return handle;
}
