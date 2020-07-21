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
const float CAMERA_SPEED = 0.1f;

class App
{
    friend class AppSingleton;
private:
    Timer appTimer{};
    Timer frameTimer{};
    GLFWwindow *wp{nullptr};

    // Entity Manager
    entt::registry EM{};
    entt::entity playerEntity{};
    double mouseXPos{}, mouseYPos{};



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

    static void errorCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar *message, const void *userParam)
    {
        std::string sourceStr{enumToString(source,
                                           ESPair{GL_DEBUG_SOURCE_API, "GL_DEBUG_SOURCE_API"},
                                           ESPair{GL_DEBUG_SOURCE_WINDOW_SYSTEM, "GL_DEBUG_SOURCE_WINDOW_SYSTEM"},
                                           ESPair{GL_DEBUG_SOURCE_SHADER_COMPILER, "GL_DEBUG_SOURCE_SHADER_COMPILER"},
                                           ESPair{GL_DEBUG_SOURCE_THIRD_PARTY, "GL_DEBUG_SOURCE_THIRD_PARTY"},
                                           ESPair{GL_DEBUG_SOURCE_APPLICATION, "GL_DEBUG_SOURCE_APPLICATION"},
                                           ESPair{GL_DEBUG_SOURCE_OTHER, "GL_DEBUG_SOURCE_OTHER"})};
        std::string typeStr{enumToString(type,
                                         ESPair{GL_DEBUG_TYPE_ERROR, "GL_DEBUG_TYPE_ERROR"},
                                         ESPair{GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR, "GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR"},
                                         ESPair{GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR, "GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR"},
                                         ESPair{GL_DEBUG_TYPE_PORTABILITY, "GL_DEBUG_TYPE_PORTABILITY"},
                                         ESPair{GL_DEBUG_TYPE_PERFORMANCE, "GL_DEBUG_TYPE_PERFORMANCE"},
                                         ESPair{GL_DEBUG_TYPE_MARKER, "GL_DEBUG_TYPE_MARKER"},
                                         ESPair{GL_DEBUG_TYPE_PUSH_GROUP, "GL_DEBUG_TYPE_PUSH_GROUP"},
                                         ESPair{GL_DEBUG_TYPE_POP_GROUP, "GL_DEBUG_TYPE_POP_GROUP"},
                                         ESPair{GL_DEBUG_TYPE_OTHER, "GL_DEBUG_TYPE_OTHER"})};
        std::string severityStr{enumToString(severity,
                                             ESPair{GL_DEBUG_SEVERITY_HIGH, "GL_DEBUG_SEVERITY_HIGH"},
                                             ESPair{GL_DEBUG_SEVERITY_MEDIUM, "GL_DEBUG_SEVERITY_MEDIUM"},
                                             ESPair{GL_DEBUG_SEVERITY_LOW, "GL_DEBUG_SEVERITY_LOW"},
                                             ESPair{GL_DEBUG_SEVERITY_NOTIFICATION, "GL_DEBUG_SEVERITY_NOTIFICATION"})};

        std::cout << "GL_ERROR: (source: " << sourceStr << ", type: " << typeStr << ", severity: " << severityStr << ", message: " << message << std::endl;
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

    AppSingleton(const AppSingleton&) = delete;
    AppSingleton(AppSingleton&&) = delete;
};