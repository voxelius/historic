/*
 * drawcmd.hpp
 * Copyright (c) 2021, Kirill GPRB.
 * All Rights Reserved.
 */
#pragma once
#include <client/render/gl/object.hpp>
#include <spdlog/spdlog.h>
namespace gl
{
struct DrawArraysCmd final {
    uint32_t vertices;
    uint32_t instances;
    uint32_t base_vertex;
    uint32_t base_instance;
};

struct DrawElementsCmd final {
    uint32_t indices;
    uint32_t instances;
    uint32_t base_index;
    uint32_t base_vertex;
    uint32_t base_instance;
};

class DrawCommand final : public Object<DrawCommand> {
public:
    DrawCommand() = default;
    DrawCommand(uint32_t mode, size_t vertices, size_t instances, size_t base_vertex, size_t base_instance);
    DrawCommand(uint32_t mode, uint32_t type, size_t indices, size_t instances, size_t base_index, size_t base_vertex, size_t base_instance);
    DrawCommand(DrawCommand &&rhs);
    DrawCommand &operator=(DrawCommand &&rhs);
    void create();
    void destroy();
    void set(uint32_t mode, size_t vertices, size_t instances, size_t base_vertex, size_t base_instance);
    void set(uint32_t mode, uint32_t type, size_t indices, size_t instances, size_t base_index, size_t base_vertex, size_t base_instance);
    void invoke();
    size_t size() const;

private:
    bool indexed { false };
    uint32_t mode { 0 };
    uint32_t type { 0 };
    size_t nv { 0 };
};
} // namespace gl

inline gl::DrawCommand::DrawCommand(uint32_t mode, size_t vertices, size_t instances, size_t base_vertex, size_t base_instance)
    : indexed(false), mode(mode), type(0), nv(0)
{
    create();
    set(mode, vertices, instances, base_vertex, base_instance);
}

inline gl::DrawCommand::DrawCommand(uint32_t mode, uint32_t type, size_t indices, size_t instances, size_t base_index, size_t base_vertex, size_t base_instance)
    : indexed(true), mode(mode), type(type), nv(0)
{
    create();
    set(mode, type, indices, instances, base_index, base_vertex, base_instance);
}

inline gl::DrawCommand::DrawCommand(gl::DrawCommand &&rhs)
{
    handle = rhs.handle;
    indexed = rhs.indexed;
    mode = rhs.mode;
    type = rhs.type;
    nv = rhs.nv;
    rhs.handle = 0;
    rhs.indexed = false;
    rhs.mode = 0;
    rhs.type = 0;
    rhs.nv = 0;
}

inline gl::DrawCommand &gl::DrawCommand::operator=(gl::DrawCommand &&rhs)
{
    gl::DrawCommand copy(std::move(rhs));
    std::swap(handle, copy.handle);
    std::swap(indexed, rhs.indexed);
    std::swap(mode, rhs.mode);
    std::swap(type, rhs.type);
    std::swap(nv, rhs.nv);
    return *this;
}

inline void gl::DrawCommand::create()
{
    destroy();
    glCreateBuffers(1, &handle);
}

inline void gl::DrawCommand::destroy()
{
    if(handle) {
        glDeleteBuffers(1, &handle);
        handle = 0;
    }
}

inline void gl::DrawCommand::set(uint32_t mode, size_t vertices, size_t instances, size_t base_vertex, size_t base_instance)
{
    nv = vertices;
    indexed = false;
    this->mode = mode;
    gl::DrawArraysCmd cmd = {};
    cmd.vertices = static_cast<uint32_t>(vertices);
    cmd.instances = static_cast<uint32_t>(instances);
    cmd.base_vertex = static_cast<uint32_t>(base_vertex);
    cmd.base_instance = static_cast<uint32_t>(base_instance);
    glNamedBufferData(handle, static_cast<GLsizeiptr>(sizeof(cmd)), &cmd, GL_STATIC_DRAW);
}

inline void gl::DrawCommand::set(uint32_t mode, uint32_t type, size_t indices, size_t instances, size_t base_index, size_t base_vertex, size_t base_instance)
{
    nv = indices;
    indexed = true;
    this->mode = mode;
    this->type = type;
    gl::DrawElementsCmd cmd = {};
    cmd.indices = static_cast<uint32_t>(indices);
    cmd.instances = static_cast<uint32_t>(instances);
    cmd.base_index = static_cast<uint32_t>(base_index);
    cmd.base_vertex = static_cast<uint32_t>(base_vertex);
    cmd.base_instance = static_cast<uint32_t>(base_instance);
    glNamedBufferData(handle, static_cast<GLsizeiptr>(sizeof(cmd)), &cmd, GL_STATIC_DRAW);
}

inline void gl::DrawCommand::invoke()
{
    glBindBuffer(GL_DRAW_INDIRECT_BUFFER, handle);
    
    if(indexed) {
        glDrawElementsIndirect(mode, type, nullptr);
        return;
    }

    glDrawArraysIndirect(mode, nullptr);
}

inline size_t gl::DrawCommand::size() const
{
    return nv;
}
