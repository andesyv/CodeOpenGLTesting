#include <glm/glm.hpp>

namespace component {
struct trans
{
    glm::vec3 pos;
    glm::vec3 rot;
    glm::vec3 scale;
};

struct mesh
{
    unsigned int VAO;
    unsigned int vertexCount;
    bool bIndices : 1;
    unsigned int indexCount : 7;

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
}