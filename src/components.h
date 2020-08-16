#ifndef COMPONENTS_H
#define COMPONENTS_H

#include <glm/glm.hpp>
#include <glm/gtc/quaternion.hpp> // glm::quat
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <type_traits>
#include <tuple>
#include <queue>

template <class _Ty, class _Container = std::deque<_Ty>>
class iqueue : public std::queue<_Ty, _Container> {
public:
    typedef typename _Container::iterator iterator;
    typedef typename _Container::const_iterator const_iterator;

    iterator begin() { return c.begin(); }
    iterator end() { return c.end(); }
    const_iterator begin() const { return c.begin(); }
    const_iterator end() const { return c.end(); }
};

struct vertex
{
    glm::vec3 pos{};
    glm::vec3 normal{};
    glm::vec2 uv{};

    // Cast operators
    operator std::tuple<glm::vec3, glm::vec3, glm::vec2>() const { return {pos, normal, uv}; }
};

static auto quatToVec(const glm::quat &q) {
    return glm::vec3{q.x, q.y, q.z};
};

namespace component {
struct trans
{
    glm::vec3 pos{};
    glm::quat rot{1.f, 0.f, 0.f, 0.f};
    glm::vec3 scale{1.f, 1.f, 1.f};

    enum FLAG : unsigned char {
        EULERROT =          1 << 0,
        SPHERE =            1 << 1,
        OBJECTCENTRIC =     1 << 2
    };
    unsigned char flags{};

    glm::mat4 mat() const {
        return glm::scale(glm::translate(glm::mat4{1.f}, pos), scale) * static_cast<glm::mat4>(rot);
    }

    static glm::mat4 createViewMat(const component::trans& comp, bool objectCentric = false) {
        if (objectCentric)
            return glm::translate(glm::mat4{1.f}, -comp.pos) * static_cast<glm::mat4>(comp.rot);
        else
            return static_cast<glm::mat4>(comp.rot) * glm::translate(glm::mat4{1.f}, -comp.pos);
    }

    /**
     * Some random calculations:
     * T_1 = T_2
     * W_1 = T_1 * R_1          <- Object centric
     * W_2 = R_2 * T_2          <- Normal
     * 
     * W_2 = W_1
     * R_2 * T_2 = T_1 * R_1
     * R_2 * T_2 * T_2^-1 = T_1 * R_1 * T_2^-1
     * R_2 = T_1 * R_1 * T_1^-1
     * 
     * R_2 * T_1 * T_1^-1 = T_1 * R_1 * T_1^-1
     * R_2 * I = R_1
     */

    static component::trans objectCentricToNormal(const component::trans& comp) {
        return component::trans{
            .pos{quatToVec(glm::conjugate(comp.rot) * glm::quat{0.f, comp.pos} * comp.rot)},
            .rot{glm::quat(-comp.pos) * comp.rot * glm::inverse(glm::quat(-comp.pos))},
            .scale{comp.scale},
            .flags{comp.flags}};
    }

    static auto objectCentricPos(const component::trans& comp) {
        return quatToVec(glm::conjugate(comp.rot) * glm::quat{0.f, comp.pos} * comp.rot);
    }

    static component::trans normalToObjectCentric(const component::trans& comp) {
        return component::trans{
            .pos{0.f, 0.f, glm::length(comp.pos)},
            .rot{glm::inverse(glm::quat(-comp.pos)) * comp.rot * glm::quat(-comp.pos)},
            .scale{comp.scale},
            .flags{comp.flags}
        };
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
    bool bDrawn = true;
};

struct metadata
{
    std::string name;
};

struct phys
{
    // Mass in kilograms
    float mass{1000.f};
    glm::dvec3 vel{0.f, 0.f, 0.f};
    bool bStatic{false};
};

struct particle
{
    iqueue<glm::vec3> pos;
};
}

#endif // COMPONENTS_H