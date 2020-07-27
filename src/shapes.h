#include "components.h"
#include <array>
#include <vector>

// https://stackoverflow.com/questions/26351587/how-to-create-stdarray-with-initialization-list-without-providing-size-directl
template <typename... T>
constexpr auto make_array(T&&... values) ->
    std::array<
       typename std::decay<
           typename std::common_type<T...>::type>::type,
       sizeof...(T)> {
    return std::array<
        typename std::decay<
            typename std::common_type<T...>::type>::type,
        sizeof...(T)>{std::forward<T>(values)...};
}

namespace shapes {
using v = vertex;

constexpr static auto axis = make_array(
    v{.normal = {1.f, 0.f, 0.f}},
    v{.pos = {1.f, 0.f, 0.f}, .normal = {1.f, 0.f, 0.f}},
    v{.normal = {0.f, 1.f, 0.f}},
    v{.pos = {0.f, 1.f, 0.f}, .normal = {0.f, 1.f, 0.f}},
    v{.normal = {0.f, 0.f, 1.f}},
    v{.pos = {0.f, 0.f, 1.f}, .normal = {0.f, 0.f, 1.f}}
);

constexpr static auto plane = make_array(
    v{.pos{0.5f, 0.f, -0.5f}, .normal{0.f, 1.f, 0.f}},
    v{.pos{-0.5f, 0.f, -0.5f}, .normal{0.f, 1.f, 0.f}},
    v{.pos{-0.5f, 0.f, 0.5f}, .normal{0.f, 1.f, 0.f}},

    v{.pos{-0.5f, 0.f, 0.5f}, .normal{0.f, 1.f, 0.f}},
    v{.pos{0.5f, 0.f, 0.5f}, .normal{0.f, 1.f, 0.f}},
    v{.pos{0.5f, 0.f, -0.5f}, .normal{0.f, 1.f, 0.f}}
);

constexpr static auto cube = make_array(
    v{.pos{-0.5f, 0.5f, 0.5f}, .normal{0.f, 0.f, 1.f}},  // top left
    v{.pos{-0.5f, -0.5f, 0.5f}, .normal{0.f, 0.f, 1.f}}, // bottom left
    v{.pos{0.5f, -0.5f, 0.5f}, .normal{0.f, 0.f, 1.f}},  // bottom right
    v{.pos{0.5f, 0.5f, 0.5f}, .normal{0.f, 0.f, 1.f}},   // top right

    v{.pos{0.5f, 0.5f, -0.5f}, .normal{0.f, 0.f, -1.f}},   // top right
    v{.pos{0.5f, -0.5f, -0.5f}, .normal{0.f, 0.f, -1.f}},  // bottom right
    v{.pos{-0.5f, -0.5f, -0.5f}, .normal{0.f, 0.f, -1.f}}, // bottom left
    v{.pos{-0.5f, 0.5f, -0.5f}, .normal{0.f, 0.f, -1.f}},  // top left

    v{.pos{0.5f, -0.5f, 0.5f}, .normal{1.f, 0.f, 0.f}},
    v{.pos{0.5f, -0.5f, -0.5f}, .normal{1.f, 0.f, 0.f}},
    v{.pos{0.5f, 0.5f, -0.5f}, .normal{1.f, 0.f, 0.f}},
    v{.pos{0.5f, 0.5f, 0.5f}, .normal{1.f, 0.f, 0.f}},

    v{.pos{-0.5f, 0.5f, 0.5f}, .normal{-1.f, 0.f, 0.f}},
    v{.pos{-0.5f, 0.5f, -0.5f}, .normal{-1.f, 0.f, 0.f}},
    v{.pos{-0.5f, -0.5f, -0.5f}, .normal{-1.f, 0.f, 0.f}},
    v{.pos{-0.5f, -0.5f, 0.5f}, .normal{-1.f, 0.f, 0.f}},

    v{.pos{0.5f, 0.5f, -0.5f}, .normal{0.f, 1.f, 0.f}},
    v{.pos{-0.5f, 0.5f, -0.5f}, .normal{0.f, 1.f, 0.f}},
    v{.pos{-0.5f, 0.5f, 0.5f}, .normal{0.f, 1.f, 0.f}},
    v{.pos{0.5f, 0.5f, 0.5f}, .normal{0.f, 1.f, 0.f}},

    v{.pos{0.5f, -0.5f, 0.5f}, .normal{0.f, -1.f, 0.f}},
    v{.pos{-0.5f, -0.5f, 0.5f}, .normal{0.f, -1.f, 0.f}},
    v{.pos{-0.5f, -0.5f, -0.5f}, .normal{0.f, -1.f, 0.f}},
    v{.pos{0.5f, -0.5f, -0.5f}, .normal{0.f, -1.f, 0.f}}
);

constexpr static auto cubeIndices = make_array(
    0u, 1u, 3u, // first Triangle
    1u, 2u, 3u, // second Triangle
    4u, 5u, 7u,
    5u, 6u, 7u,
    8u, 9u, 11u,
    9u, 10u, 11u,
    12u, 13u, 15u,
    13u, 14u, 15u,
    16u, 17u, 19u,
    17u, 18u, 19u,
    20u, 21u, 23u,
    21u, 22u, 23u
);

// Points in CCW order
void sphereSide(std::vector<vertex>& vList, std::array<glm::vec3, 4> points, unsigned int subdivisions = 0) {
    if (subdivisions) {
        for (unsigned int i{0}; i < 4; ++i) {
            std::array<glm::vec3, 4> newPoints{};
            newPoints[i % 4] = points[i];
            newPoints[(i + 1) % 4] = points[i] + (points[(i + 1) % 4] - points[i]) * 0.5f;
            newPoints[(i + 2) % 4] = points[i] + (points[(i + 2) % 4] - points[i]) * 0.5f;
            newPoints[(i + 3) % 4] = points[i] + (points[(i + 3) % 4] - points[i]) * 0.5f;
            sphereSide(vList, newPoints, subdivisions - 1);
        }
    } else {
        for (unsigned int i{0}; i < 4; ++i)
            points[i] = glm::normalize(points[i]);

        vList.insert(vList.end(), {
            {.pos{points[0]}, .normal{points[0]}},
            {.pos{points[1]}, .normal{points[1]}},
            {.pos{points[2]}, .normal{points[2]}},

            {.pos{points[2]}, .normal{points[2]}},
            {.pos{points[3]}, .normal{points[3]}},
            {.pos{points[0]}, .normal{points[0]}}
        });
    }
}



std::vector<vertex> cubeSphere(unsigned int subdivisions = 0) {
    static constexpr glm::vec3 p[] = {
        {-1.f, 1.f, 1.f},
        {-1.f, -1.f, 1.f},
        {1.f, -1.f, 1.f},
        {1.f, 1.f, 1.f},

        {1.f, 1.f, 1.f},
        {1.f, -1.f, 1.f},
        {1.f, -1.f, -1.f},
        {1.f, 1.f, -1.f},

        {1.f, 1.f, -1.f},
        {1.f, -1.f, -1.f},
        {-1.f, -1.f, -1.f},
        {-1.f, 1.f, -1.f},

        {-1.f, 1.f, -1.f},
        {-1.f, -1.f, -1.f},
        {-1.f, -1.f, 1.f},
        {-1.f, 1.f, 1.f},

        {-1.f, 1.f, -1.f},
        {-1.f, 1.f, 1.f},
        {1.f, 1.f, 1.f},
        {1.f, 1.f, -1.f},

        {-1.f, -1.f, 1.f},
        {-1.f, -1.f, -1.f},
        {1.f, -1.f, -1.f},
        {1.f, -1.f, 1.f}
    };

    std::vector<vertex> vertices;
    for (unsigned int side{0}; side < 6; ++side) {
        std::array<glm::vec3, 4> points;
        for (unsigned int j{0}; j < 4; ++j)
            points[j] = p[side * 4 + j];

        sphereSide(vertices, points, subdivisions);
    }

    return vertices;
}
}