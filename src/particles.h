#ifndef PARTICLES_H
#define PARTICLES_H

#include "components.h"
#include "shader.h"
#include <vector>
#include <glad/glad.h>

template <std::size_t trailSize = 10>
class Particles {
private:
    unsigned int b;
    std::size_t pCount{0};
    Shader particleShader;

public:
    Particles(std::size_t particleCount = 0)
        : pCount{particleCount}, particleShader{"src/shaders/particle.vert", "src/shaders/particle.frag", {
            {"pcount", std::to_string(particleCount)},
            {"tlength", std::to_string(trailSize)}
        }}
    {
        glGenBuffers(1, &b);
        glBindBuffer(GL_UNIFORM_BUFFER, b);
        glBufferData(GL_UNIFORM_BUFFER, pCount * trailSize * sizeof(glm::vec3), nullptr, GL_STATIC_DRAW);
        glBindBufferBase(GL_UNIFORM_BUFFER, 0, b);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);

        // Temporary solution. Uniform bufer objects cannot handle more than 16KB. :s
        assert(pCount * trailSize * sizeof(glm::vec3) < 16 * 1024);
    }

    // Prevent move and copy functionality
    Particles(const Particles&) = delete;
    Particles(Particles&&) = delete;
    void operator=(const Particles&) = delete;
    void operator=(Particles&&) = delete;

    template <typename T>
    static void updatePos(T&& view) {
        for (auto ent{view.begin()}; ent != view.end(); ++ent) {
            auto& [t, p] = view.get<component::trans, component::particle>(*ent);

            p.pos.push(t.pos);
            // Ensure there are 10 items in the queue
            if (p.pos.size() != trailSize + 1)
                for (std::size_t i{trailSize - p.pos.size()}; 0 < i; --i)
                    p.pos.push(t.pos);
            else
            // Pop back
                p.pos.pop();
        }
    }

    template <typename T>
    void render(T&& view, const component::mesh& mesh, const component::camera& camera) {
        // std::cout << "pCount: " << pCount << ", view.size(): " << view.size() << std::endl;
        // assert(view.size() == pCount);
        // std::vector<glm::vec3> positions;
        // positions.reserve(count);
        glBindBuffer(GL_UNIFORM_BUFFER, b);
        std::array<glm::vec3, trailSize> positions{};
        constexpr auto blockSize = trailSize * sizeof(glm::vec3);
        unsigned int i{0};
        // view.each([&](auto ent, const auto& p){
        //     // positions.insert(positions.end(), p.pos.begin(), p.pos.end());
        //     std::copy(p.pos.begin(), p.pos.end(), positions.begin());
        //     glBufferSubData(GL_UNIFORM_BUFFER, i * blockSize, blockSize, positions.data());
        //     ++i;    
        // });

        for (auto ent{view.begin()}; ent != view.end() && i < pCount; ++ent, ++i) {
            const auto& p = view.get<component::particle>(*ent);

            std::copy(p.pos.begin(), p.pos.end(), positions.begin());
            glBufferSubData(GL_UNIFORM_BUFFER, i * blockSize, blockSize, positions.data());
        }

        glBindVertexArray(mesh.VAO);
        const auto& s = particleShader.get();
        glUseProgram(s);
        glUniformMatrix4fv(glGetUniformLocation(s, "uProj"), 1, GL_FALSE, glm::value_ptr(camera.proj));
        glUniformMatrix4fv(glGetUniformLocation(s, "uView"), 1, GL_FALSE, glm::value_ptr(camera.view));
        glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.vertexCount, pCount * trailSize);

        glBindBuffer(GL_UNIFORM_BUFFER, 0);
    }

    ~Particles() {
        glDeleteBuffers(1, &b);
    }
};


#endif