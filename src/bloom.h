#ifndef BLOOM_H
#define BLOOM_H

#include <glad/glad.h>
#include "shader.h"
#include "components.h"
#include <vector>
#include <memory>
#include <optional>


class Bloom {
private:
    GLsizei width, height;
    static constexpr unsigned int blurBufferDivisor = 2;

    unsigned int inputBuf, iTex, iDepth;
    unsigned int base;
    unsigned int bTex[2], bDepth[2];
    unsigned int pingpong[2];
    unsigned int ppTex[2];
    unsigned lastPing = 0;

    std::optional<unsigned> q, qVBO;

    void createQuad() {
        if (q) return;

        q.emplace();
        qVBO.emplace();
        glGenVertexArrays(1, &*q);
        glBindVertexArray(*q);
        glGenBuffers(1, &*qVBO);
        glBindBuffer(GL_ARRAY_BUFFER, *qVBO);

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

    void initBuffers() {
        glGenFramebuffers(1, &inputBuf);
        glBindFramebuffer(GL_FRAMEBUFFER, inputBuf);
        glGenTextures(1, &iTex);
        glBindTexture(GL_TEXTURE_2D, iTex);
        glViewport(0, 0, width, height);
        glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width, height, 0, GL_RGBA, GL_FLOAT, nullptr);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
        glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
        glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, iTex, 0);

        glGenRenderbuffers(1, &iDepth);
        glBindRenderbuffer(GL_RENDERBUFFER, iDepth);
        glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH24_STENCIL8, width, height);
        glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_STENCIL_ATTACHMENT, GL_RENDERBUFFER, iDepth);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Framebuffer failed with status: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
            return;
        }



        glGenFramebuffers(1, &base);
        glBindFramebuffer(GL_FRAMEBUFFER, base);
        glViewport(0, 0, width, height);
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
        unsigned attachments[]{ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 };
        glDrawBuffers(2, attachments);

        if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE) {
            std::cout << "Framebuffer failed with status: " << glCheckFramebufferStatus(GL_FRAMEBUFFER) << std::endl;
            return;
        }

        glGenFramebuffers(2, pingpong);
        glGenTextures(2, ppTex);
        for (unsigned int i{0}; i < 2; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpong[i]);
            glViewport(0, 0, width, height);
            glBindTexture(GL_TEXTURE_2D, ppTex[i]);
            glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, width / blurBufferDivisor, height / blurBufferDivisor, 0, GL_RGBA, GL_FLOAT, nullptr);
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
    
public:
    std::shared_ptr<Shader> splitShader{std::make_shared<Shader>("src/shaders/postprocessing/pass.vert", "src/shaders/postprocessing/split.frag")};
    std::shared_ptr<Shader> blurShader{std::make_shared<Shader>("src/shaders/postprocessing/pass.vert", "src/shaders/postprocessing/blur.frag")};
    std::shared_ptr<Shader> combineShader{std::make_shared<Shader>("src/shaders/postprocessing/pass.vert", "src/shaders/postprocessing/combine.frag")};

    // Default constructor
    Bloom(GLsizei screenWidth = 800, GLsizei screenHeight = 600)
        : width{screenWidth}, height{screenHeight}
    {
        initBuffers();
    }

    // Default copy constructor
    Bloom(const Bloom& rhs)
        : width{rhs.width}, height{rhs.height},
        splitShader{rhs.splitShader}, blurShader{rhs.blurShader}, combineShader{rhs.combineShader} 
    {
        initBuffers();
    }

    // Copy constructor with new width and height
    Bloom(const Bloom& rhs, GLsizei newWidth, GLsizei newHeight)
        : width{newWidth}, height{newHeight},
        splitShader{rhs.splitShader}, blurShader{rhs.blurShader}, combineShader{rhs.combineShader} 
    {
        initBuffers();
    }

    Bloom(Bloom&&) = delete;

    // Custom move constructor that invalidates rhs's shader pointers and screenspaced quad.
    Bloom(Bloom&& rhs, GLsizei newWidth, GLsizei newHeight)
        : width{newWidth}, height{newHeight},
        splitShader{std::move(rhs.splitShader)}, blurShader{std::move(rhs.blurShader)}, combineShader{std::move(rhs.combineShader)} 
    {
        q.swap(rhs.q);
        qVBO.swap(rhs.qVBO);
        initBuffers();
    }

    // Initial write to buffer
    unsigned int input() const {
        return inputBuf;
    }

    void split() {
        glBindFramebuffer(GL_FRAMEBUFFER, base);
        glReadBuffer(GL_COLOR_ATTACHMENT0);

        glDisable(GL_DEPTH_TEST);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(splitShader->get());
        glBindVertexArray(*q);
        glBindTexture(GL_TEXTURE_2D, iTex);

        render();

        glBindFramebuffer(GL_READ_FRAMEBUFFER, base);
        glReadBuffer(GL_COLOR_ATTACHMENT1);
        glBindFramebuffer(GL_DRAW_FRAMEBUFFER, pingpong[0]);
        glBlitFramebuffer(0, 0, width, height, 0, 0, width / blurBufferDivisor, height / blurBufferDivisor, GL_COLOR_BUFFER_BIT, GL_LINEAR);
        glViewport(0, 0, width / blurBufferDivisor, height / blurBufferDivisor);
    }

    void blur(unsigned int amount = 10) {
        glBindVertexArray(*q);
        glUseProgram(blurShader->get());
        bool horizontal{false};
        for (unsigned int i{0}; i < amount; ++i) {
            glBindFramebuffer(GL_FRAMEBUFFER, pingpong[!horizontal]);
            glDisable(GL_DEPTH_TEST);
            glClear(GL_COLOR_BUFFER_BIT);
            glUniform1i(glGetUniformLocation(blurShader->get(), "horizontal"), horizontal);
            glBindTexture(GL_TEXTURE_2D, ppTex[horizontal]);
            lastPing = horizontal = !horizontal;

            render();
        }

        glViewport(0, 0, width, height);
        glBlitFramebuffer(0, 0, width / blurBufferDivisor, height / blurBufferDivisor, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
    }

    void combine() {
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        glBindVertexArray(*q);
        glClear(GL_COLOR_BUFFER_BIT);
        glUseProgram(combineShader->get());

        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D, bTex[0]);
        glUniform1i(glGetUniformLocation(combineShader->get(), "tex"), 0);

        glActiveTexture(GL_TEXTURE1);
        glBindTexture(GL_TEXTURE_2D, ppTex[lastPing]);
        glUniform1i(glGetUniformLocation(combineShader->get(), "bloom"), 1);

        render();

        glActiveTexture(GL_TEXTURE0);
    }

    void doTheThing() {
        split();
        blur();
        combine();

        glBindVertexArray(0);
    }

    ~Bloom (){
        if (q) {
            glDeleteBuffers(1, &*qVBO);
            glDeleteBuffers(1, &*q);
            q = std::nullopt;
            qVBO = std::nullopt;
        }

        glDeleteTextures(2, ppTex);
        glDeleteFramebuffers(2, pingpong);
        glDeleteTextures(2, bTex);
        glDeleteFramebuffers(1, &base);
        glDeleteTextures(1, &iTex);
        glDeleteRenderbuffers(1, &iDepth);
        glDeleteFramebuffers(1, &inputBuf);
    }
};

#endif