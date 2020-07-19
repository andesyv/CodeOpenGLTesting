#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glad/glad.h>
#include <type_traits>

struct vertex
{
    glm::vec3 pos{};
    glm::vec3 normal{};
    glm::vec2 uv{};
};

namespace component {
struct trans
{
    glm::vec3 pos{};
    glm::vec3 rot{};
    glm::vec3 scale{1.f, 1.f, 1.f};

    glm::mat4 mat() const {
        return glm::translate(glm::scale(glm::mat4{1.f}, scale), pos);
    }
};

struct camera
{
    glm::mat4 proj;
    glm::mat4 view;
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
    unsigned int vertexCount;
    bool bIndices : 1;
    unsigned int indexCount : 7;
    GLenum drawMode{GL_TRIANGLES};

    mesh(unsigned int _VAO = 0, unsigned int _vCount = 0)
        : VAO{_VAO}, vertexCount{_vCount}, bIndices{false}
    {
    }

    mesh(unsigned int _VAO, unsigned int _vCount, unsigned int _iCount)
        : VAO{_VAO}, vertexCount{_vCount}, bIndices{true}, indexCount{_iCount}
    {
    }
};

struct mat
{
    int shader;
};

struct metadata
{
    std::string name;
};
}