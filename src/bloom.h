#ifndef BLOOM_H
#define BLOOM_H

#include <glad/glad.h>
#include "shader.h"
#include "components.h"
#include <vector>


class Bloom {
private:
    GLsizei width, height;

    // 0 is base, 1 and 2 is ping pong
    unsigned int base;
    unsigned int bTex[2];
    unsigned int pingpong[2];
    unsigned int ppTex[2];
    unsigned lastPing = 0;

    Shader splitShader{"src/shaders/postprocessing/pass.vert", "src/shaders/postprocessing/split.frag"};
    Shader blurShader{"src/shaders/postprocessing/pass.vert", "src/shaders/postprocessing/blur.frag"};
    Shader combineShader{"src/shaders/postprocessing/pass.vert", "src/shaders/postprocessing/combine.frag"};
    unsigned q, qVBO;

    void createQuad() {
        glGenVertexArrays(1, &q);
        glBindVertexArray(q);
        glGenBuffers(1, &qVBO);
        glBindBuffer(GL_ARRAY_BUFFER, qVBO);

        std::vector<vertex> vertices{
            {.pos{-1.f, -1.f, 0.f}, .uv{0.f, 0.f}},
            {.pos{1.f, -1.f, 0.f}, .uv{1.f, 0.f}},
            {.pos{1.f, 1.f, 0.f}, .uv{1.f, 1.f}},

            {.pos{1.f, 1.f, 0.f}, .uv{1.f, 1.f}},
            {.pos{-1.f, 1.f, 0.f}, .uv{0.f, 1.f}},
            {.pos{-1.f, -1.f, 0.f}, .uv{0.f, 0.f}}
        };
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertex) * vertices.size(), vertices.data(), GL_STATIC_DRAW);
        glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), nullptr);
        // glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(3 * sizeof(float)));
        glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(6 * sizeof(float)));
        glEnableVertexAttribArray(0);
        // glEnableVertexAttribArray(1);
        glEnableVertexAttribArray(2);
    }

    void render() {
        glDrawArrays(GL_TRIANGLES, 0, 6);
    }
    
public:
    unsigned int inputTex;

    Bloom(unsigned int input, GLsizei screenWidth = 800, GLsizei screenHeight = 600)
        : width{screenWidth}, height{screenHeight}, inputTex{input}
    {
        glGenFramebuffers(1, &base);
        glBindFramebuffer(GL_FRAMEBUFFER, base);
        glGenTextures(2, bTex);
        for (unsigned int i{0}; i < 2; ++i) {
            glBindTexture(GL_TEXTURE_2D, bTex[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, GL_TEXTURE_2D, bTex[i], 0);
        }
        unsigned attachments[] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        glViewport(0, 0, width, height);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Framebuffer failed with status: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
            return;
        }

        glGenFramebuffers(2, pingpong);
        glGenTextures(2, ppTex);
        for (unsigned int i{0}; i < 2; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpong[i]);
            glBindTexture(GL_TEXTURE_2D, ppTex[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
            glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

            glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, ppTex[i], 0);
        }

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Framebuffer failed with status: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
            return;
        }

        createQuad();
    }

    void split() {
        glBindFramebuffer(GL_FRAMEBUFFER, base);
        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(splitShader.get());
        glBindVertexArray(q);
        glBindTexture(GL_TEXTURE_2D, inputTex);

        render();
    }

    void blur(unsigned int amount = 10) {
        glBindVertexArray(q);
        glUseProgram(blurShader.get());
        bool horizontal{false};
        for (unsigned int i{0}; i < amount; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpong[!horizontal]);
            glDisable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT);
            glUniform1i(glGetUniformLocation(blurShader.get(), "horizontal"), horizontal);
            glBindTexture(GL_TEXTURE_2D, i == 0 ? bTex[1] : ppTex[horizontal]);
            lastPing = horizontal = !horizontal;

            render();
        }
    }

    void combine() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(q);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(combineShader.get());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bTex[0]);
        glUniform1i(glGetUniformLocation(combineShader.get(), "tex"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ppTex[lastPing]);
        glUniform1i(glGetUniformLocation(combineShader.get(), "bloom"), 1);

        render();

        glActiveTexture(GL_TEXTURE0);
    }

    void doTheThing() {
        split();
        blur();
        combine();

        glBindVertexArray(0);
        glEnable(GL_DEPTH_TEST);
    }

    ~Bloom (){
        glDeleteBuffers(1, &qVBO);
        glDeleteBuffers(1, &q);

        glDeleteTextures(2, ppTex);
        glDeleteFramebuffers(2, pingpong);
        glDeleteTextures(2, bTex);
        glDeleteFramebuffers(1, &base);
    }
};

#endif