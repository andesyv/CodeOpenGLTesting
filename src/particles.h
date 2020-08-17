#ifndef PARTICLES_H
#define PARTICLES_H

#include "components.h"
#include "shader.h"
#include <vector>
#include <glad/glad.h>

template <std::size_t pCount, std::size_t trailSize = 10>
class Particles {
private:
    unsigned int b;
    Shader particleShader;
    typedef typename glm::vec4 pPosT;

public:
    Particles()
        : particleShader{"src/shaders/particle.vert", "src/shaders/particle.frag", {
            {"pcount", std::to_string(pCount)},
            {"tlength", std::to_string(trailSize)}
        }}
    {
        glGenBuffers(1, &b);
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, b);
        glBufferData(GL_SHADER_STORAGE_BUFFER, pCount * (trailSize + 2) * sizeof(pPosT), nullptr, GL_STATIC_DRAW);
        glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, b);
        glBindBuffer(GL_UNIFORM_BUFFER, 0);
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
            
            p.scale = t.scale;
        }
    }

    template <typename T>
    void updateShaderData(T&& view) {
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, b);
        std::array<pPosT, trailSize> positions{};
        std::array<pPosT, pCount> scales{};
        std::array<pPosT, pCount> colors{};
        GLintptr bufferOffset{0};
        constexpr auto blockSize = sizeof(positions);
        unsigned int i{0};

        for (auto ent{view.begin()}; ent != view.end() && i < pCount; ++ent, ++i) {
            const auto& p = view.get<component::particle>(*ent);

            std::transform(p.pos.begin(), p.pos.end(), positions.begin(), [](const glm::vec3& p){
                return glm::vec4{p, 0.0};
            });
            glBufferSubData(GL_SHADER_STORAGE_BUFFER, bufferOffset, blockSize, positions.data());
            bufferOffset += blockSize;
        }

        auto it = scales.begin();
        auto cit = colors.begin();
        unsigned particleCount{0};
        view.each([&](auto ent, const component::particle& p, const component::mat& m){
            *it = glm::vec4{p.scale, 0.f};
            *cit = glm::vec4{m.color, 1.f};
            ++it;
            ++cit;
        });
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, bufferOffset, sizeof(scales), scales.data());
        bufferOffset += sizeof(scales);
        glBufferSubData(GL_SHADER_STORAGE_BUFFER, bufferOffset, sizeof(colors), colors.data());
        glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
    }

    void render(const component::mesh& mesh, const component::camera& camera) {
        glBindVertexArray(mesh.VAO);
        const auto& s = particleShader.get();
        glUseProgram(s);
        glUniformMatrix4fv(glGetUniformLocation(s, "uProj"), 1, GL_FALSE, glm::value_ptr(camera.proj));
        glUniformMatrix4fv(glGetUniformLocation(s, "uView"), 1, GL_FALSE, glm::value_ptr(camera.view));
        glDrawArraysInstanced(GL_TRIANGLES, 0, mesh.vertexCount, pCount * trailSize);
    }

    ~Particles() {
        glDeleteBuffers(1, &b);
    }
};


#endif