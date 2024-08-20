#include "client/graphics/vertex_array.hpp"

namespace attributes {
constexpr i32 POSITION = 0;
constexpr i32 TEXCOORDS = 1;
constexpr i32 COLOR = 2;
}

VertexArray::VertexArray() {
    glGenBuffers(1, &this->vbo);
    glGenVertexArrays(1, &this->vao);
}

VertexArray::~VertexArray() {
    glDeleteVertexArrays(1, &this->vao);
    glDeleteBuffers(1, &this->vbo);
}

void VertexArray::AddUnitQuad() {
    this->Add(Vec2{0.0f}, Vec2{1.0f, 1.0f}, Vec2{0.0f}, Vec2{1.0f, 1.0f}, Color{255, 255, 255, 255});
}

void VertexArray::AddVertex(Vec2 pos, Color color) {
    Vertex vertex;
    vertex.position = pos;
    vertex.tex_coords = Vec2{};
    vertex.color = color;

    this->dirty = true;
    this->num_vertices = 0;
    this->vertices.push_back(vertex);
}

void VertexArray::Add(Vec2 source_pos, Vec2 source_size, Vec2 dest_pos, Vec2 dest_size, Color color) {
    auto source_top_left     = source_pos + Vec2{0.0f, 0.0f};
    auto source_top_right    = source_pos + Vec2{source_size.x, 0.0f};
    auto source_bottom_right = source_pos + Vec2{source_size.x, source_size.y};
    auto source_bottom_left  = source_pos + Vec2{0.0f, source_size.y};
    auto dest_top_left       = dest_pos   + Vec2{0.0f,   0.0f};
    auto dest_top_right      = dest_pos   + Vec2{dest_size.x, 0.0f};
    auto dest_bottom_right   = dest_pos   + Vec2{dest_size.x, dest_size.y};
    auto dest_bottom_left    = dest_pos   + Vec2{0.0f, dest_size.y};

    this->AddQuad(
        source_top_left,
        source_top_right,
        source_bottom_right,
        source_bottom_left,
        dest_top_left,
        dest_top_right,
        dest_bottom_right,
        dest_bottom_left,
        color);
}

void VertexArray::AddRotated(Vec2 source_pos, Vec2 source_size, Vec2 dest_pos, Vec2 dest_size, i16 rotation, Color color) {
    auto half_size = dest_size / 2.0f;
    auto rot = glm::radians(static_cast<f32>(rotation));

    auto source_top_left     = source_pos + Vec2{0.0f, 0.0f};
    auto source_top_right    = source_pos + Vec2{source_size.x, 0.0f};
    auto source_bottom_right = source_pos + Vec2{source_size.x, source_size.y};
    auto source_bottom_left  = source_pos + Vec2{0.0f, source_size.y};
    auto dest_top_left       = dest_pos   + Vec2{-half_size.x, -half_size.y};
    auto dest_top_right      = dest_pos   + Vec2{+half_size.x, -half_size.y};
    auto dest_bottom_right   = dest_pos   + Vec2{+half_size.x, +half_size.y};
    auto dest_bottom_left    = dest_pos   + Vec2{-half_size.x, +half_size.y};
    dest_top_left     = half_size + dest_pos + glm::rotate(dest_top_left     - dest_pos, rot);
    dest_top_right    = half_size + dest_pos + glm::rotate(dest_top_right    - dest_pos, rot);
    dest_bottom_right = half_size + dest_pos + glm::rotate(dest_bottom_right - dest_pos, rot);
    dest_bottom_left  = half_size + dest_pos + glm::rotate(dest_bottom_left  - dest_pos, rot);

    this->AddQuad(
        source_top_left,
        source_top_right,
        source_bottom_right,
        source_bottom_left,
        dest_top_left,
        dest_top_right,
        dest_bottom_right,
        dest_bottom_left,
        color);
}

void VertexArray::AddQuad(
    Vec2 source_top_left,
    Vec2 source_top_right,
    Vec2 source_bottom_right,
    Vec2 source_bottom_left,
    Vec2 dest_top_left,
    Vec2 dest_top_right,
    Vec2 dest_bottom_right,
    Vec2 dest_bottom_left,
    Color color) {
    Vertex tris[6];
    tris[0].position = dest_top_right;
    tris[0].tex_coords = source_top_right;
    tris[0].color = color;
    tris[1].position = dest_bottom_right;
    tris[1].tex_coords = source_bottom_right;
    tris[1].color = color;
    tris[2].position = dest_top_left;
    tris[2].tex_coords = source_top_left;
    tris[2].color = color;
    tris[3].position = dest_bottom_right;
    tris[3].tex_coords = source_bottom_right;
    tris[3].color = color;
    tris[4].position = dest_bottom_left;
    tris[4].tex_coords = source_bottom_left;
    tris[4].color = color;
    tris[5].position = dest_top_left;
    tris[5].tex_coords = source_top_left;
    tris[5].color = color;

    this->AddTriangles(tris);
}

void VertexArray::AddTriangles(const Vertex vertices[6]) {
    this->dirty = true;
    this->num_vertices = 0;
    this->vertices.insert(this->vertices.end(), vertices, vertices + 6);
}

void VertexArray::Commit() {
    if (!this->dirty) {
        return;
    }

    this->dirty = false;
    this->num_vertices = this->vertices.size();

    glBindVertexArray(this->vao); {
        glBindBuffer(GL_ARRAY_BUFFER, this->vbo); {
            // Copy the vertices into the vbo
            glBufferData(GL_ARRAY_BUFFER, sizeof(Vertex) * this->vertices.size(), this->vertices.data(), GL_STATIC_DRAW);

            // Setup attributes
            glVertexAttribPointer(attributes::POSITION,  2, GL_FLOAT,         GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, position)));
            glVertexAttribPointer(attributes::TEXCOORDS, 2, GL_FLOAT,         GL_FALSE, sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, tex_coords)));
            glVertexAttribPointer(attributes::COLOR,     4, GL_UNSIGNED_BYTE, GL_TRUE,  sizeof(Vertex), reinterpret_cast<void *>(offsetof(Vertex, color)));
            glEnableVertexAttribArray(attributes::POSITION);
            glEnableVertexAttribArray(attributes::TEXCOORDS);
            glEnableVertexAttribArray(attributes::COLOR);
        } glBindBuffer(GL_ARRAY_BUFFER, 0);
    } glBindVertexArray(0);

    this->vertices.clear();
}

//
// Instance_Array
//

InstanceArray::InstanceArray() {
    glGenBuffers(1, &this->ssbo);
}

InstanceArray::~InstanceArray() {
    glDeleteBuffers(1, &this->ssbo);
}

void InstanceArray::Add(Mat4 model) {
    this->models.push_back(model);
}

void InstanceArray::Add(Vec2 position, Vec2 size) {
    // NOTE(janh) position, scale and rotation are only here to construct the model matrix.
    // They don't have any impact on the vertex uvs.
    Mat4 model{1.0f};
    model = glm::translate(model, Vec3{position, 0.0f});
    model = glm::scale(model, Vec3{size, 1.0f});

    this->Add(model);
}

void InstanceArray::Add(Vec2 position, Vec2 size, f32 rotation) {
    Mat4 model{1.0f};
    model = glm::translate(model, Vec3{position, 0.0f});
    model = glm::translate(model, Vec3{size / 2.0f, 0.0f});
    model = glm::rotate(model, glm::radians(rotation), Vec3{0.0f, 0.0f, 1.0f});
    model = glm::translate(model, Vec3{-size / 2.0f, 0.0f});
    model = glm::scale(model, Vec3{size, 1.0f});

    this->Add(model);
}

void InstanceArray::Add(Vec2 position, Vec2 size, Vec2 rotation_center, f32 rotation) {
    Mat4 model{1.0f};
    model = glm::translate(model, Vec3{position, 0.0f});
    model = glm::translate(model, Vec3{rotation_center, 0.0f});
    model = glm::rotate(model, glm::radians(rotation), Vec3{0.0f, 0.0f, 1.0f});
    model = glm::translate(model, Vec3{-rotation_center, 0.0f});
    model = glm::scale(model, Vec3{size, 1.0f});

    this->Add(model);
}

void InstanceArray::SendData(const VertexArray &vertex_array) {
    glBindVertexArray(vertex_array.vao); {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, this->ssbo); {
            glBufferData(GL_SHADER_STORAGE_BUFFER, sizeof(Mat4) * this->models.size(), this->models.data(), GL_STATIC_DRAW); // Really GL_STATIC_DRAW???
            glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, this->ssbo);
        }
    } glBindVertexArray(0);
}

