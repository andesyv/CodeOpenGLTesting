#ifndef APP_H
#define APP_H

#include <glad/glad.h>
#include <GLFW/glfw3.h>
#include <array>
#include <chrono> // Timers
#include <iostream>
#include <entt/entt.hpp> // https://github.com/skypjack/entt
#include <glm/glm.hpp>   // https://github.com/g-truc/glm
#include <tuple>

#include "shader.h"
#include "timer.h"
#include "components.h"

// settings
const unsigned int SCR_WIDTH = 800;
const unsigned int SCR_HEIGHT = 600;
const float SCR_NEAR = 1.f;
const float SCR_FAR = 1000.f;
const float CAMERA_ROTATION_SPEED = 0.1f;

class App
{
    friend class AppSingleton;
private:
    Timer appTimer{};
    Timer frameTimer{};
    GLFWwindow *wp{nullptr};
    entt::entity screenSpacedQuad;
    unsigned int fbo, rbTex, fbDepth;
    unsigned int screenSpaceVAO, screenSpaceVBO;
    Shader *UIShader = nullptr;
    float cameraSpeed{1.f};

    // Entity Manager
    entt::registry EM{};
    entt::entity playerEntity{};
    double mouseXPos{}, mouseYPos{};
    float mouseWheelDist{0.f};
    // Timestep multiplum. A multiplier added to the deltaTime each frame.
    float timeDilation{1.f};
    bool bPause{false};
    bool bSpacePressed{false};
    glm::ivec2 screenSize{SCR_WIDTH, SCR_HEIGHT};



    int initGLFW();
    int initOpenGL();    
    void showFPS();
    // process all input: query GLFW whether relevant keys are pressed/released this frame and react accordingly
    void processInput(float deltaTime = 1.f);
    void gameloop();

public:
    App();
    
    // Recursive init function that uses rewards to calculate progress
    int init(int currentReward = 0);
    int exec();
    void setupScene();
    void cleanupScene();


    // Static members:

    // glfw: whenever the wp size changed (by OS or user resize) this callback function executes
    static void framebuffer_size_callback(GLFWwindow *wp, int width, int height);
    static void errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam);
    static void scroll_callback(GLFWwindow *wp, double xoffset, double yoffset) {
        static_cast<App*>(glfwGetWindowUserPointer(wp))->mouseWheelDist += yoffset;
    }



    // Templates
    template <unsigned int n>
    static std::string enumToString(GLenum arg, const std::array<std::pair<GLenum, std::string>, n> &params)
    {
        for (unsigned int i{0}; i < params.size(); ++i)
            if (params[i].first == arg)
                return params[i].second;

        return "UNDEFINED";
    }

    typedef std::pair<GLenum, std::string> ESPair;

    template <typename... T>
    static std::string enumToString(GLenum arg, T... params)
    {
        constexpr auto n = sizeof...(T);
        return enumToString<n>(arg, std::array<std::pair<GLenum, std::string>, n>{params...});
    }
};


/**
 * Singleton class for App instances.
 * Should exist one instance across threads
 * which should be able to access a App instance
 * on the specific thread.
 */
class AppSingleton
{
    friend App::App();
private:
    std::vector<App*> Instances;
    AppSingleton() = default;

public:
    static AppSingleton& get() {
        static AppSingleton singletonInstance{};
        return singletonInstance;
    }

    App* find(GLFWwindow* wp) {
        for (const auto& inst : Instances)
            if (inst != nullptr && inst->wp != nullptr && inst->wp == wp)
                return inst;
        
        return nullptr;
    }

    std::vector<App*> getInstances() { return Instances; }

    AppSingleton(const AppSingleton&) = delete;
    AppSingleton(AppSingleton&&) = delete;
};

#endif // APP_H