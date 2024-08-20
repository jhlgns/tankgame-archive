#pragma once

#include "common/common.hpp"
#include "client/graphics/glad.h"
#include "client/graphics/texture.hpp"

struct VertexArray {
    VertexArray();
    VertexArray(const VertexArray &) = delete;
    VertexArray& operator=(const VertexArray &) = delete;
    VertexArray(VertexArray &&) = delete;
    VertexArray& operator=(VertexArray &&) = delete;
    ~VertexArray();
    void AddUnitQuad();
    void AddVertex(Vec2 pos, Color color);
    void Add(Vec2 source_pos, Vec2 source_size, Vec2 dest_pos, Vec2 dest_size, Color color);
    void AddRotated(Vec2 source_pos, Vec2 source_size, Vec2 dest_pos, Vec2 dest_size, i16 rotation, Color color);
    void AddQuad(
        Vec2 source_top_left,
        Vec2 source_top_right,
        Vec2 source_bottom_right,
        Vec2 source_bottom_left,
        Vec2 dest_top_left,
        Vec2 dest_top_right,
        Vec2 dest_bottom_right,
        Vec2 dest_bottom_left,
        Color color);
    void AddTriangles(const Vertex vertices[6]);
    void Commit();

    bool dirty = false;
    Array<Vertex> vertices;
    size_t num_vertices = 0;
    GLuint vao;
    GLuint vbo;
};

struct InstanceArray {
    InstanceArray();
    InstanceArray(const InstanceArray &) = delete;
    InstanceArray& operator=(const InstanceArray &) = delete;
    InstanceArray(InstanceArray &&) = delete;
    InstanceArray& operator=(InstanceArray &&) = delete;
    ~InstanceArray();
    void Add(Vec2 position, Vec2 size);
    void Add(Vec2 position, Vec2 size, f32 rotation);
    void Add(Vec2 position, Vec2 size, Vec2 rotation_center, f32 rotation);
    void Add(Mat4 model);
    void SendData(const VertexArray &vertex_array);

    GLuint ssbo;
    Array<Mat4> models;
};
