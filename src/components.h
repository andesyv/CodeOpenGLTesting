#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp> // glm::quat
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <type_traits>
#include <tuple>

struct vertex
{
    glm::vec3 pos{};
    glm::vec3 normal{};
    glm::vec2 uv{};

    // Cast operators
    operator std::tuple<glm::vec3, glm::vec3, glm::vec2>() const { return {pos, normal, uv}; }
};

namespace component {
struct trans
{
    glm::vec3 pos{};
    glm::quat rot{1.f, 0.f, 0.f, 0.f};
    glm::vec3 scale{1.f, 1.f, 1.f};

    glm::mat4 mat() const {
        return glm::scale(glm::translate(glm::mat4{1.f}, pos), scale) * static_cast<glm::mat4>(rot);
    }

    static glm::mat4 createViewMat(const component::trans& comp) {
        return static_cast<glm::mat4>(comp.rot) * glm::translate(glm::mat4{1.f}, -comp.pos);
    }

    static glm::vec3 rightVector(const glm::quat& rot)
    {
        return {
            1.f - 2.f * (rot.y * rot.y + rot.z * rot.z),
            2.f * (rot.x * rot.y + rot.w * rot.z),
            2.f * (rot.x * rot.z - rot.w * rot.y)
        };
        // return static_cast<glm::mat3>(glm::conjugate(rot)) * glm::vec3{1.f, 0.f, 0.f};
    }

    static glm::vec3 upVector(const glm::quat &rot)
    {
        return {
            2.f * (rot.x * rot.y - rot.w * rot.z),
            1.f - 2.f * (rot.x * rot.x + rot.z * rot.z),
            2.f * (rot.y * rot.z + rot.w * rot.x)
        };
    }

    static glm::vec3 forwardVector(const glm::quat &rot)
    {
        return {
            2.f * (rot.x * rot.z + rot.w * rot.y),
            2.f * (rot.y * rot.z - rot.w * rot.x),
            1.f - 2.f * (rot.x * rot.x + rot.y * rot.y)
        };
    }

    glm::vec3 right() const { return rightVector(rot); }
    glm::vec3 up() const { return upVector(rot); }
    glm::vec3 forward() const { return forwardVector(rot); }

    /**
     * 2a^2 - 1 + 2b^2, 2bc + 2ad, 2bd - 2ac
     * 2 * w * w - 1 + 2 * x * x, 2 * x * y + 2 * w * z, 2 * x * z - 2 * w * y
     * 2 * (w * w + x * x) - 1, 2 * (x * y + w * z), 2 * (x * z - w * y)
     */
};

struct camera
{
    glm::mat4 proj;
    glm::mat4 view;
    float pitch{0.f}, yaw{0.f};
    float FOV{45.f};
};

// template <typename T>
// bool anyOf(T val, T test)
// {
//     return val == test;
// }

// template <typename T, typename... Args /*,
//     typename = std::enable_if_t<std::conjunction_v<std::is_same<T, Args>...>>*/>
// bool anyOf(T val, T test, Args... args)
// {
//     return anyOf(val, test) || anyOf(val, args...);
// }

struct mesh
{
    /**
     * Vertex Array Object(renderinfo),
     * Vertex Buffer Object(vertex info),
     * Index/Element Buffer Object(intex or triangle info)
     */
    unsigned int VAO, VBO, IBO;
    unsigned int vertexCount{0};
    bool bIndices : 1;
    unsigned int indexCount : 7;
    GLenum drawMode{GL_TRIANGLES};

    mesh() : bIndices{false}, vertexCount{0}, indexCount{0} {}
};

struct mat
{
    int shader;
    glm::vec3 color{1.f, 1.f, 1.f};
};

struct metadata
{
    std::string name;
};

struct phys
{
    // Mass in kilograms
    float mass{1000.f};
    glm::vec3 vel{0.f, 0.f, 0.f};
};
}

#endif // COMPONENTS_H