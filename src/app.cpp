#include "app.h"
#include <glm/gtc/type_ptr.hpp> // For glm::value_ptr()
#include <glm/gtc/matrix_transform.hpp> // For glm::perspective
#include <glm/gtc/quaternion.hpp>       // glm::quat

App::App()
{
    AppSingleton::get().Instances.push_back(this);
}

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

void App::showFPS()
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

void App::processInput(float deltaTime)
{
    if (glfwGetKey(wp, GLFW_KEY_ESCAPE) == GLFW_PRESS)
        glfwSetWindowShouldClose(wp, true);

    
    double xPos, yPos;
    glfwGetCursorPos(wp, &xPos, &yPos);
    double deltaX{xPos - mouseXPos}, deltaY{yPos - mouseYPos};
    deltaX *= deltaTime * CAMERA_SPEED;
    deltaY *= deltaTime * CAMERA_SPEED;
    mouseXPos = xPos;
    mouseYPos = yPos;


    auto& [pTrans, pCamera] = EM.get<component::trans, component::camera>(playerEntity);
    if (glfwGetMouseButton(wp, 1) == GLFW_PRESS)
    {
        if (glfwGetKey(wp, GLFW_KEY_SPACE) == GLFW_PRESS || glfwGetKey(wp, GLFW_KEY_E) == GLFW_PRESS)
            pTrans.pos += pTrans.upVector(glm::conjugate(pTrans.rot)) * deltaTime;

        if (glfwGetKey(wp, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS || glfwGetKey(wp, GLFW_KEY_Q) == GLFW_PRESS)
            pTrans.pos -= pTrans.upVector(glm::conjugate(pTrans.rot)) * deltaTime;

        if (glfwGetKey(wp, GLFW_KEY_D) == GLFW_PRESS)
            pTrans.pos += pTrans.rightVector(glm::conjugate(pTrans.rot)) * deltaTime;

        if (glfwGetKey(wp, GLFW_KEY_A) == GLFW_PRESS)
            pTrans.pos -= pTrans.rightVector(glm::conjugate(pTrans.rot)) * deltaTime;

        // W and S are inverted because z in the coordinate system is inverted
        if (glfwGetKey(wp, GLFW_KEY_W) == GLFW_PRESS)
            pTrans.pos -= pTrans.forwardVector(glm::conjugate(pTrans.rot)) * deltaTime;

        if (glfwGetKey(wp, GLFW_KEY_S) == GLFW_PRESS)
            pTrans.pos += pTrans.forwardVector(glm::conjugate(pTrans.rot)) * deltaTime;
        
        pCamera.pitch += deltaY;
        pCamera.yaw += deltaX;
        pTrans.rot = glm::quat{std::cosf(pCamera.pitch * 0.5f), std::sinf(pCamera.pitch * 0.5f), 0.f, 0.f} *
                     glm::quat{std::cosf(pCamera.yaw * 0.5f), 0.f, std::sinf(pCamera.yaw * 0.5f), 0.f};
        pCamera.view = component::trans::createViewMat(pTrans);
    }
}

void App::gameloop()
{
    // Find time since last frame
    const auto deltaTime = frameTimer.elapsed<std::chrono::milliseconds>() * 0.001f;
    frameTimer.reset();

    showFPS();

    // input
    // -----
    processInput(deltaTime);

    // render
    // ------
    glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

    glUseProgram(0);
    unsigned int currentShader{0};

    const auto& [camera, playerTrans] = EM.get<component::camera, component::trans>(playerEntity);

    auto view = EM.view<component::mesh, component::mat>();
    for (const auto &entity : view)
    {
        glm::mat4 modelMat{1.f};
        // (Structured bindings ftw!! ENTT is so cool)
        auto &[mesh, material] = view.get<component::mesh, component::mat>(entity);

        // Set shader and shader-params (only if not already set)
        if (material.shader != currentShader) {
            currentShader = material.shader;
            glUseProgram(material.shader);
            glUniformMatrix4fv(glGetUniformLocation(material.shader, "uProj"), 1, GL_FALSE, glm::value_ptr(camera.proj));
            glUniformMatrix4fv(glGetUniformLocation(material.shader, "uView"), 1, GL_FALSE, glm::value_ptr(camera.view));
        }

        // Assign a model matrix if it exist
        if (EM.has<component::trans>(entity)) {
            auto& transform = EM.get<component::trans>(entity);
            transform.rot *= glm::quat{std::cosf(deltaTime * 0.5f), glm::vec3{0.f, std::sinf(deltaTime * 0.5f), 0.f}};
            // transform.pos.x += deltaTime * 0.1f;
            modelMat = transform.mat();
        }
        glUniformMatrix4fv(glGetUniformLocation(material.shader, "uModel"), 1, GL_FALSE, glm::value_ptr(modelMat));

        glBindVertexArray(mesh.VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
        if (mesh.bIndices)
            glDrawElements(mesh.drawMode, mesh.indexCount, GL_UNSIGNED_INT, 0);
        else
            glDrawArrays(mesh.drawMode, 0, mesh.vertexCount);
    }
    glBindVertexArray(0); // no need to unbind it every time

    // glfw: swap buffers and poll IO events (keys pressed/released, mouse moved etc.)
    // -------------------------------------------------------------------------------
    glfwSwapBuffers(wp);
    glfwPollEvents();
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
    // Reset cursor position
    glfwGetCursorPos(wp, &mouseXPos, &mouseYPos);

    // ================== Setup scene ====================================
    setupScene();

    std::cout << "Setup took " << appTimer.elapsed<std::chrono::milliseconds>() << "ms." << std::endl;
    appTimer.reset();
    frameTimer.reset();

    // render loop
    // -----------
    while (!glfwWindowShouldClose(wp))
    {
        gameloop();
    }

    // // optional: de-allocate all resources once they've outlived their purpose:
    // // ------------------------------------------------------------------------
    cleanupScene();

    // glfw: terminate, clearing all previously allocated GLFW resources.
    // ------------------------------------------------------------------
    glfwTerminate();
    return 0;
}

void App::setupScene()
{
    Shader defaultShader{"src/shaders/default.vert", "src/shaders/default.frag"};
    Shader colorShader{"src/shaders/default.vert", "src/shaders/color.frag"};

    glEnable(GL_CULL_FACE);
    // glFrontFace()
    glEnable(GL_DEPTH_TEST);
    glDepthFunc(GL_LESS);

    // Setup player
    auto entity = EM.create();
    EM.emplace<component::trans>(entity).pos.z = 5.f;
    EM.emplace<component::metadata>(entity, "player");
    auto &camera = EM.emplace<component::camera>(entity);
    int width, height;
    glfwGetFramebufferSize(wp, &width, &height);
    /**
     * glm::perspective makes a right hand coordinate system,
     * meaning that x is right, y is up and negative z is forward.
     * (z is flipped from world space to window space)
     */
    camera.proj = glm::perspective(glm::radians(camera.FOV), static_cast<float>(width) / height, 0.1f, 100.f);
    camera.view = glm::mat4{1.f};
    playerEntity = entity;



    // ------------- Axis: ------------------------------
    entity = EM.create();
    EM.emplace<component::mat>(entity, colorShader.get());
    EM.emplace<component::metadata>(entity, "axis");
    auto mesh = &EM.emplace<component::mesh>(entity);
    // EM.emplace<component::trans>(entity);

    std::vector<vertex> vertices{
        {.normal = {1.f, 0.f, 0.f}},
        {.pos = {1.f, 0.f, 0.f}, .normal = {1.f, 0.f, 0.f}},
        {.normal = {0.f, 1.f, 0.f}},
        {.pos = {0.f, 1.f, 0.f}, .normal = {0.f, 1.f, 0.f}},
        {.normal = {0.f, 0.f, 1.f}},
        {.pos = {0.f, 0.f, 1.f}, .normal = {0.f, 0.f, 1.f}}};
    unsigned int VAO, VBO, EBO;
    glGenVertexArrays(1, &mesh->VAO);
    glBindVertexArray(mesh->VAO);

    glGenBuffers(1, &mesh->VBO);
    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), nullptr);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    mesh->vertexCount = static_cast<unsigned int>(vertices.size());
    mesh->drawMode = GL_LINES;




    // ----------- Plane: ------------------------------
    auto cubeEnt = entity = EM.create();
    EM.emplace<component::mat>(entity, defaultShader.get());
    mesh = &EM.emplace<component::mesh>(entity);
    EM.emplace<component::trans>(entity);
    EM.emplace<component::metadata>(entity, "cube");
    // set up vertex data (and buffer(s)) and configure vertex attributes
    // ------------------------------------------------------------------
    vertices = {
        {.pos{-0.5f, 0.5f, 0.5f}, .normal{0.f, 0.f, 1.f}},  // top left
        {.pos{-0.5f, -0.5f, 0.5f}, .normal{0.f, 0.f, 1.f}}, // bottom left
        {.pos{0.5f, -0.5f, 0.5f}, .normal{0.f, 0.f, 1.f}},  // bottom right
        {.pos{0.5f, 0.5f, 0.5f}, .normal{0.f, 0.f, 1.f}},   // top right

        {.pos{0.5f, 0.5f, -0.5f}, .normal{0.f, 0.f, -1.f}},   // top right
        {.pos{0.5f, -0.5f, -0.5f}, .normal{0.f, 0.f, -1.f}},  // bottom right
        {.pos{-0.5f, -0.5f, -0.5f}, .normal{0.f, 0.f, -1.f}}, // bottom left
        {.pos{-0.5f, 0.5f, -0.5f}, .normal{0.f, 0.f, -1.f}},  // top left

        {.pos{0.5f, -0.5f, 0.5f}, .normal{1.f, 0.f, 0.f}},
        {.pos{0.5f, -0.5f, -0.5f}, .normal{1.f, 0.f, 0.f}},
        {.pos{0.5f, 0.5f, -0.5f}, .normal{1.f, 0.f, 0.f}},
        {.pos{0.5f, 0.5f, 0.5f}, .normal{1.f, 0.f, 0.f}},

        {.pos{-0.5f, 0.5f, 0.5f}, .normal{-1.f, 0.f, 0.f}},
        {.pos{-0.5f, 0.5f, -0.5f}, .normal{-1.f, 0.f, 0.f}},
        {.pos{-0.5f, -0.5f, -0.5f}, .normal{-1.f, 0.f, 0.f}},
        {.pos{-0.5f, -0.5f, 0.5f}, .normal{-1.f, 0.f, 0.f}},

        {.pos{0.5f, 0.5f, -0.5f}, .normal{0.f, 1.f, 0.f}},
        {.pos{-0.5f, 0.5f, -0.5f}, .normal{0.f, 1.f, 0.f}},
        {.pos{-0.5f, 0.5f, 0.5f}, .normal{0.f, 1.f, 0.f}},
        {.pos{0.5f, 0.5f, 0.5f}, .normal{0.f, 1.f, 0.f}},

        {.pos{0.5f, -0.5f, 0.5f}, .normal{0.f, -1.f, 0.f}},
        {.pos{-0.5f, -0.5f, 0.5f}, .normal{0.f, -1.f, 0.f}},
        {.pos{-0.5f, -0.5f, -0.5f}, .normal{0.f, -1.f, 0.f}},
        {.pos{0.5f, -0.5f, -0.5f}, .normal{0.f, -1.f, 0.f}}
    };

    typedef std::array<unsigned int, 3> tri;
    std::vector<tri> indices{
        {0, 1, 3}, // first Triangle
        {1, 2, 3},  // second Triangle
        {4, 5, 7},
        {5, 6, 7},
        {8, 9, 11},
        {9, 10, 11},
        {12, 13, 15},
        {13, 14, 15},
        {16, 17, 19},
        {17, 18, 19},
        {20, 21, 23},
        {21, 22, 23}
    };

    glGenVertexArrays(1, &mesh->VAO);
    glGenBuffers(1, &mesh->VBO);
    glGenBuffers(1, &mesh->IBO);
    // bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
    glBindVertexArray(mesh->VAO);

    glBindBuffer(GL_ARRAY_BUFFER, mesh->VBO);
    glBufferData(GL_ARRAY_BUFFER, vertices.size() * sizeof(vertex), vertices.data(), GL_STATIC_DRAW);

    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, mesh->IBO);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(tri), indices.data(), GL_STATIC_DRAW);
    // GLint bufferSize;
    // glGetBufferParameteriv(GL_ELEMENT_ARRAY_BUFFER, GL_BUFFER_SIZE, &bufferSize);
    // std::cout << "buffer size: " << bufferSize << std::endl;

    glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)0);
    glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(3 * sizeof(float)));
    glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(vertex), (void *)(6 * sizeof(float)));
    glEnableVertexAttribArray(0);
    glEnableVertexAttribArray(1);
    glEnableVertexAttribArray(2);

    mesh->vertexCount = static_cast<unsigned int>(vertices.size());
    mesh->bIndices = true;
    // std::tuple_size to get length of array. Surprisingly that it doesn't already exist in std::array
    // https://stackoverflow.com/questions/21936507/why-isnt-stdarraysize-static
    mesh->indexCount = static_cast<unsigned int>(indices.size() * std::tuple_size<tri>::value);
    std::cout << "Index count: " << mesh->indexCount << ", also size of triangle is: " << sizeof(tri) << std::endl;

    // // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    // glBindBuffer(GL_ARRAY_BUFFER, 0);

    // remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
    //glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);





    


    for (unsigned int i{0}, max{100}; i < max; ++i) {
        entity = EM.create();
        EM.emplace<component::mat>(entity, colorShader.get());
        auto& trans = EM.emplace<component::trans>(entity);
        float deg = i * 6.28f / max;
        trans.pos = glm::vec3{std::cosf(deg) * 5.f, 0.f, std::sinf(deg) * 5.f};
        trans.rot = glm::quat{std::cosf(deg * 0.5f), 0.f, std::sinf(deg * 0.5f), 0.f};
        // Copy the mesh component (use same VAO)
        EM.emplace<component::mesh>(entity, EM.get<component::mesh>(cubeEnt));
        EM.emplace<component::metadata>(entity, std::string{"plane "}.append(std::to_string(i)));
    }

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    glBindVertexArray(0);

    // uncomment this call to draw in wireframe polygons.
    // glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
}

void App::cleanupScene()
{
    auto view = EM.view<component::mesh>();
    for (auto entity : view)
    {
        auto &mesh = view.get<component::mesh>(entity);

        if (mesh.bIndices)
            glDeleteBuffers(1, &mesh.IBO);
        
        glDeleteBuffers(1, &mesh.VBO);
        glDeleteVertexArrays(1, &mesh.VAO);
    }
}

void App::framebuffer_size_callback(GLFWwindow *wp, int width, int height)
{
    // make sure the viewport matches the new wp dimensions; note that width and
    // height will be significantly larger than specified on retina displays.
    glViewport(0, 0, width, height);

    auto app = AppSingleton::get().find(wp);
    assert(app != nullptr);
    auto &camera = app->EM.get<component::camera>(app->playerEntity);
    /**
         * glm::perspective makes a right hand coordinate system,
         * meaning that x is right, y is up and negative z is forward.
         * (z is flipped from world space to window space)
         */
    camera.proj = glm::perspective(glm::radians(camera.FOV), static_cast<float>(width) / height, 0.1f, 100.f);
}