#include "app.h"

int App::initGLFW()
{
    // glfw: initialize and configure
    // ------------------------------
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

#ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE); // uncomment this statement to fix compilation on OS X
#endif

    // glfw wp creation
    // --------------------
    wp = glfwCreateWindow(SCR_WIDTH, SCR_HEIGHT, "CodeOpenGLTesting", NULL, NULL);
    if (wp == NULL)
    {
        std::cout << "Failed to create GLFW wp" << std::endl;
        glfwTerminate();
        return -1;
    }
    glfwSetWindowUserPointer(wp, this);
    glfwSetFramebufferSizeCallback(wp, framebuffer_size_callback);
    return 1;
}

int App::initOpenGL()
{
    glfwMakeContextCurrent(wp);

    // glad: load all OpenGL function pointers
    // ---------------------------------------
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    int versionMajor, versionMinor;
    glGetIntegerv(GL_MAJOR_VERSION, &versionMajor);
    glGetIntegerv(GL_MINOR_VERSION, &versionMinor);
    std::cout << "Running OpenGL version: " << versionMajor << "." << versionMinor << std::endl;

    glEnable(GL_DEBUG_OUTPUT);
    // glEnable(GL_DEBUG_OUTPUT_SYNCHRONOUS); // If you want to ensure the error happens exactly after the error on the same thread.
    glDebugMessageCallback(&errorCallback, nullptr);
    return 1;
}

int App::init(int currentReward)
{
    if (1 < currentReward)
        return 3;
    else if (0 < currentReward)
        return init(currentReward + 2 * initOpenGL());
    else if (-1 < currentReward)
        return init(currentReward + initGLFW());
    else
        return 0;
}

int App::exec()
{
    if (!init())
        return -1;

    glfwMakeContextCurrent(wp);

    // ================== Setup scene ====================================

    Shader defaultShader{"src/shaders/default.vert", "src/shaders/default.frag"};

    auto entity = EM.create();
    auto &mat = EM.emplace<component::mat>(entity, defaultShader.get());
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    float vertices[] = {
        0.5f, 0.5f, 0.0f,   // top right
        0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f, // bottom left
        -0.5f, 0.5f, 0.0f   // top left
    };
    unsigned int indices[] = {
        // note that we start from 0!
        0, 1, 3, // first Triangle
        1, 2, 3  // second Triangle
    };
    unsigned int VBO, VAO, EBO;
    glGenVertexArrays(1, &VAO);
    glGenBuffers(1, &VBO);
    glGenBuffers(1, &EBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(VAO);

    glBindBuffer(GL_ARRAY_BUFFER, VBO);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
    glEnableVertexAttribArray(0);

    // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    EM.emplace<component::mesh>(entity, VAO, static_cast<unsigned int>(sizeof(vertices)), static_cast<unsigned int>(sizeof(indices)));

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);

    std::cout << "Setup took " << appTimer.elapsed<std::chrono::milliseconds>() << "ms." << std::endl;
    appTimer.reset();

    Timer frameTimer{};

    // render loop
    // -----------
    while (!glfwWindowShouldClose(wp))
    {
        // Find time since last frame
        const auto deltaTime = frameTimer.elapsed<std::chrono::milliseconds>() * 0.001f;
        frameTimer.reset();

        showFPS(wp);

        // input
        // -----
        processInput(wp);

        // render
        // ------
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        auto view = EM.view<component::mesh, component::mat>();
        for (const auto &entity : view)
        {
            // (Structured bindings ftw!! ENTT is so cool)
            auto &[mesh, material] = view.get<component::mesh, component::mat>(entity);

            // draw our first triangle
            glUseProgram(material.shader);
            glBindVertexArray(mesh.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
            if (mesh.bIndices)
                glDrawElements(GL_TRIANGLES, mesh.indexCount, GL_UNSIGNED_INT, 0);
            else
                glDrawArrays(GL_TRIANGLES, 0, mesh.vertexCount);
        }
        glBindVertexArray(0); // no need to unbind it every time

        // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
        // -------------------------------------------------------------------------------
        glfwSwapBuffers(wp);
        glfwPollEvents();
    }

    // optional: de-allocate all resources once they've outlived their purpose:
    // ------------------------------------------------------------------------
    glDeleteVertexArrays(1, &VAO);
    glDeleteBuffers(1, &VBO);
    glDeleteBuffers(1, &EBO);

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void App::showFPS(GLFWwindow *wp)
{
    static Timer timer{};
    static unsigned int frameCount{0};
    auto elapsed = timer.elapsed<std::chrono::milliseconds>();
    if (elapsed >= 1000)
    {
        const auto fps = frameCount * 1000.f / elapsed;
        std::string title{"CodeOpenGLTesting, fps: " + std::to_string(fps)};
        glfwSetWindowTitle(wp, title.c_str());
        frameCount = 0;
        timer.reset();
    }
    ++frameCount;
}

void App::processInput(GLFWwindow *wp)
{
    if (glfwGetKey(wp, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(wp, true);
}